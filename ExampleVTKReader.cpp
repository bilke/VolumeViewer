// STD includes
#include <iostream>
#include <string>
#include <math.h>

// OpenGL/Motif includes
#include <GL/GLContextData.h>
#include <GL/gl.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/Menu.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/WidgetManager.h>

// VRUI includes
#include <Vrui/Application.h>
#include <Vrui/Tool.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

// VTK includes
#include <ExternalVTKWidget.h>
#include <vtkActor.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataSetMapper.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkImageData.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkLight.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkXMLImageDataReader.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

// ExampleVTKReader includes
#include "BaseLocator.h"
#include "ClippingPlane.h"
#include "ClippingPlaneLocator.h"
#include "ExampleVTKReader.h"
#include "FlashlightLocator.h"

//----------------------------------------------------------------------------
ExampleVTKReader::DataItem::DataItem(void)
{
  /* Initialize VTK renderwindow and renderer */
  this->externalVTKWidget = vtkSmartPointer<ExternalVTKWidget>::New();
  this->actor = vtkSmartPointer<vtkActor>::New();
  this->externalVTKWidget->GetRenderer()->AddActor(this->actor);
  this->actorOutline = vtkSmartPointer<vtkActor>::New();
  this->externalVTKWidget->GetRenderer()->AddActor(this->actorOutline);
  this->actorVolume = vtkSmartPointer<vtkVolume>::New();
  this->externalVTKWidget->GetRenderer()->AddVolume(actorVolume);
  this->colorFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
  this->opacityFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
  this->propertyVolume = vtkSmartPointer<vtkVolumeProperty>::New();
  this->flashlight = vtkSmartPointer<vtkLight>::New();
  this->externalVTKWidget->GetRenderer()->AddLight(this->flashlight);
}

//----------------------------------------------------------------------------
ExampleVTKReader::DataItem::~DataItem(void)
{
}

//----------------------------------------------------------------------------
ExampleVTKReader::ExampleVTKReader(int& argc,char**& argv)
  :Vrui::Application(argc,argv),
  analysisTool(0),
  ClippingPlanes(NULL),
  FileName(0),
  FirstFrame(true),
  FlashlightDirection(0),
  FlashlightPosition(0),
  FlashlightSwitch(0),
  mainMenu(NULL),
  NumberOfClippingPlanes(6),
  Opacity(1.0),
  opacityValue(NULL),
  Outline(true),
  renderingDialog(NULL),
  RepresentationType(2),
  Verbose(false),
  Volume(false)
{
  /* Create the user interface: */
  renderingDialog = createRenderingDialog();
  mainMenu=createMainMenu();
  Vrui::setMainMenu(mainMenu);

  this->DataDimensions = new int[3];
  this->DataBounds = new double[6];
  this->DataOrigin = new double[6];
  this->DataSpacing = new double[3];
  this->DataScalarRange = new double[2];

  this->FlashlightSwitch = new int[1];
  this->FlashlightSwitch[0] = 0;
  this->FlashlightPosition = new double[3];
  this->FlashlightDirection = new double[3];

  /* Initialize the clipping planes */
  ClippingPlanes = new ClippingPlane[NumberOfClippingPlanes];
  for(int i = 0; i < NumberOfClippingPlanes; ++i)
    {
    ClippingPlanes[i].setAllocated(false);
    ClippingPlanes[i].setActive(false);
    }
}

//----------------------------------------------------------------------------
ExampleVTKReader::~ExampleVTKReader(void)
{
  if(this->DataDimensions)
    {
    delete[] this->DataDimensions;
    }
  if(this->DataBounds)
    {
    delete[] this->DataBounds;
    }
  if(this->DataOrigin)
    {
    delete[] this->DataOrigin;
    }
  if(this->DataSpacing)
    {
    delete[] this->DataSpacing;
    }
  if(this->DataScalarRange)
    {
    delete[] this->DataScalarRange;
    }
  if(this->FlashlightSwitch)
    {
    delete[] this->FlashlightSwitch;
    }
  if(this->FlashlightPosition)
    {
    delete[] this->FlashlightPosition;
    }
  if(this->FlashlightDirection)
    {
    delete[] this->FlashlightDirection;
    }
}

//----------------------------------------------------------------------------
void ExampleVTKReader::setFileName(const char* name)
{
  if(this->FileName && name && (!strcmp(this->FileName, name)))
    {
    return;
    }
  if(this->FileName && name)
    {
    delete [] this->FileName;
    }
  this->FileName = new char[strlen(name) + 1];
  strcpy(this->FileName, name);
}

//----------------------------------------------------------------------------
const char* ExampleVTKReader::getFileName(void)
{
  return this->FileName;
}

//----------------------------------------------------------------------------
void ExampleVTKReader::setVerbose(bool verbose)
{
  this->Verbose = verbose;
}

//----------------------------------------------------------------------------
bool ExampleVTKReader::getVerbose(void)
{
  return this->Verbose;
}

//----------------------------------------------------------------------------
GLMotif::PopupMenu* ExampleVTKReader::createMainMenu(void)
{
  GLMotif::PopupMenu* mainMenuPopup = new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
  mainMenuPopup->setTitle("Main Menu");
  GLMotif::Menu* mainMenu = new GLMotif::Menu("MainMenu",mainMenuPopup,false);

  GLMotif::CascadeButton* representationCascade = new GLMotif::CascadeButton("RepresentationCascade", mainMenu,
    "Representation");
  representationCascade->setPopup(createRepresentationMenu());

  GLMotif::CascadeButton* analysisToolsCascade = new GLMotif::CascadeButton("AnalysisToolsCascade", mainMenu,
    "Analysis Tools");
  analysisToolsCascade->setPopup(createAnalysisToolsMenu());

  GLMotif::Button* centerDisplayButton = new GLMotif::Button("CenterDisplayButton",mainMenu,"Center Display");
  centerDisplayButton->getSelectCallbacks().add(this,&ExampleVTKReader::centerDisplayCallback);

  GLMotif::ToggleButton * showRenderingDialog = new GLMotif::ToggleButton("ShowRenderingDialog", mainMenu,
    "Rendering");
  showRenderingDialog->setToggle(false);
  showRenderingDialog->getValueChangedCallbacks().add(this, &ExampleVTKReader::showRenderingDialogCallback);

  mainMenu->manageChild();
  return mainMenuPopup;
}

//----------------------------------------------------------------------------
GLMotif::Popup* ExampleVTKReader::createRepresentationMenu(void)
{
  const GLMotif::StyleSheet* ss = Vrui::getWidgetManager()->getStyleSheet();

  GLMotif::Popup* representationMenuPopup = new GLMotif::Popup("representationMenuPopup", Vrui::getWidgetManager());
  GLMotif::SubMenu* representationMenu = new GLMotif::SubMenu("representationMenu", representationMenuPopup, false);

  GLMotif::ToggleButton* showOutline=new GLMotif::ToggleButton("ShowOutline",representationMenu,"Outline");
  showOutline->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeRepresentationCallback);
  showOutline->setToggle(true);

  GLMotif::RadioBox* representation_RadioBox = new GLMotif::RadioBox("Representation RadioBox",representationMenu,true);

  GLMotif::ToggleButton* showPoints=new GLMotif::ToggleButton("ShowPoints",representation_RadioBox,"Points");
  showPoints->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeRepresentationCallback);
  GLMotif::ToggleButton* showWireframe=new GLMotif::ToggleButton("ShowWireframe",representation_RadioBox,"Wireframe");
  showWireframe->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeRepresentationCallback);
  GLMotif::ToggleButton* showSurface=new GLMotif::ToggleButton("ShowSurface",representation_RadioBox,"Surface");
  showSurface->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeRepresentationCallback);
  GLMotif::ToggleButton* showSurfaceWEdges=new GLMotif::ToggleButton("ShowSurfaceWEdges",representation_RadioBox,"Surface With Edges");
  showSurfaceWEdges->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeRepresentationCallback);
  GLMotif::ToggleButton* showVolume=new GLMotif::ToggleButton("ShowVolume",representation_RadioBox,"Volume");
  showVolume->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeRepresentationCallback);

  representation_RadioBox->setSelectionMode(GLMotif::RadioBox::ATMOST_ONE);
  representation_RadioBox->setSelectedToggle(showSurface);

  representationMenu->manageChild();
  return representationMenuPopup;
}

//----------------------------------------------------------------------------
GLMotif::Popup * ExampleVTKReader::createAnalysisToolsMenu(void)
{
  const GLMotif::StyleSheet* ss = Vrui::getWidgetManager()->getStyleSheet();

  GLMotif::Popup * analysisToolsMenuPopup = new GLMotif::Popup("analysisToolsMenuPopup", Vrui::getWidgetManager());
  GLMotif::SubMenu* analysisToolsMenu = new GLMotif::SubMenu("representationMenu", analysisToolsMenuPopup, false);

  GLMotif::RadioBox * analysisTools_RadioBox = new GLMotif::RadioBox("analysisTools", analysisToolsMenu, true);

  GLMotif::ToggleButton* showClippingPlane=new GLMotif::ToggleButton("ClippingPlane",analysisTools_RadioBox,"Clipping Plane");
  showClippingPlane->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeAnalysisToolsCallback);
  GLMotif::ToggleButton* showFlashlight=new GLMotif::ToggleButton("Flashlight",analysisTools_RadioBox,"Flashlight");
  showFlashlight->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeAnalysisToolsCallback);
  GLMotif::ToggleButton* showOther=new GLMotif::ToggleButton("Other",analysisTools_RadioBox,"Other");
  showOther->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeAnalysisToolsCallback);

  analysisTools_RadioBox->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
  analysisTools_RadioBox->setSelectedToggle(showClippingPlane);

  analysisToolsMenu->manageChild();
  return analysisToolsMenuPopup;
}

//----------------------------------------------------------------------------
GLMotif::PopupWindow* ExampleVTKReader::createRenderingDialog(void) {
  const GLMotif::StyleSheet& ss = *Vrui::getWidgetManager()->getStyleSheet();
  GLMotif::PopupWindow * dialogPopup = new GLMotif::PopupWindow("RenderingDialogPopup", Vrui::getWidgetManager(),
    "Rendering Dialog");
  GLMotif::RowColumn * dialog = new GLMotif::RowColumn("RenderingDialog", dialogPopup, false);
  dialog->setOrientation(GLMotif::RowColumn::HORIZONTAL);

  /* Create opacity slider */
  GLMotif::Slider* opacitySlider = new GLMotif::Slider("OpacitySlider", dialog, GLMotif::Slider::HORIZONTAL,
    ss.fontHeight*10.0f);
  opacitySlider->setValue(Opacity);
  opacitySlider->setValueRange(0.0, 1.0, 0.1);
  opacitySlider->getValueChangedCallbacks().add(this, &ExampleVTKReader::opacitySliderCallback);
  opacityValue = new GLMotif::TextField("OpacityValue", dialog, 6);
  opacityValue->setFieldWidth(6);
  opacityValue->setPrecision(3);
  opacityValue->setValue(Opacity);

  dialog->manageChild();
  return dialogPopup;
}

//----------------------------------------------------------------------------
void ExampleVTKReader::frame(void)
{
  if(this->FirstFrame)
    {
    /* Compute the data center and Radius once */
    this->Center[0] = (this->DataBounds[0] + this->DataBounds[1])/2.0;
    this->Center[1] = (this->DataBounds[2] + this->DataBounds[3])/2.0;
    this->Center[2] = (this->DataBounds[4] + this->DataBounds[5])/2.0;

    this->Radius = sqrt((this->DataBounds[1] - this->DataBounds[0])*
                        (this->DataBounds[1] - this->DataBounds[0]) +
                        (this->DataBounds[3] - this->DataBounds[2])*
                        (this->DataBounds[3] - this->DataBounds[2]) +
                        (this->DataBounds[5] - this->DataBounds[4])*
                        (this->DataBounds[5] - this->DataBounds[4]));
    /* Scale the Radius */
    this->Radius *= 0.75;
    /* Initialize Vrui navigation transformation: */
    centerDisplayCallback(0);
    this->FirstFrame = false;
    }
}

//----------------------------------------------------------------------------
void ExampleVTKReader::initContext(GLContextData& contextData) const
{
  /* Create a new context data item */
  DataItem* dataItem = new DataItem();
  contextData.addDataItem(this, dataItem);

  vtkNew<vtkDataSetMapper> mapper;
  dataItem->actor->SetMapper(mapper.GetPointer());
  vtkNew<vtkOutlineFilter> dataOutline;
  vtkNew<vtkPolyDataMapper> mapperOutline;

  mapperOutline->SetInputConnection(dataOutline->GetOutputPort());
  dataItem->actorOutline->SetMapper(mapperOutline.GetPointer());
  dataItem->actorOutline->GetProperty()->SetColor(1,1,1);

  vtkNew<vtkSmartVolumeMapper> mapperVolume;

  if(this->FileName)
    {
    vtkNew<vtkXMLImageDataReader> reader;
    reader->SetFileName(this->FileName);
    reader->Update();

    mapper->SetInputConnection(reader->GetOutputPort());
    reader->GetOutput()->GetDimensions(this->DataDimensions);
    reader->GetOutput()->GetBounds(this->DataBounds);
    reader->GetOutput()->GetOrigin(this->DataOrigin);
    reader->GetOutput()->GetSpacing(this->DataSpacing);
    reader->GetOutput()->GetScalarRange(this->DataScalarRange);
    dataOutline->SetInputConnection(reader->GetOutputPort());
    mapperVolume->SetInputConnection(reader->GetOutputPort());
    }
  else
    {
    vtkNew<vtkImageData> imageData;
    this->DataDimensions[0] = 3;
    this->DataDimensions[1] = 3;
    this->DataDimensions[2] = 3;
    this->DataOrigin[0] = -1;
    this->DataOrigin[1] = -1;
    this->DataOrigin[2] = -1;
    this->DataSpacing[0] = 1;
    this->DataSpacing[1] = 1;
    this->DataSpacing[2] = 1;
    imageData->SetDimensions(this->DataDimensions);
    imageData->SetOrigin(this->DataOrigin);
    imageData->SetSpacing(this->DataSpacing);
    imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    for (int i = 0; i < this->DataDimensions[0]; ++i)
      {
      for (int j = 0; j < this->DataDimensions[1]; ++j)
        {
        for (int k = 0; k < this->DataDimensions[2]; ++k)
          {
          unsigned char * pixel = static_cast<unsigned char *>(
            imageData->GetScalarPointer(i,j,k));
          pixel[0] = 255.0;
          }
        }
      }
    imageData->GetBounds(this->DataBounds);
    imageData->GetScalarRange(this->DataScalarRange);
    mapper->SetInputData(imageData.GetPointer());
    dataOutline->SetInputData(imageData.GetPointer());
    mapperVolume->SetInputData(imageData.GetPointer());
    }

  dataItem->colorFunction->AddRGBPoint(this->DataScalarRange[0], 0.0, 0.0, 0.0);
  dataItem->colorFunction->AddRGBPoint(this->DataScalarRange[1], 1.0, 1.0, 1.0);
  dataItem->opacityFunction->AddPoint(this->DataScalarRange[0], 0.0);
  dataItem->opacityFunction->AddPoint(this->DataScalarRange[1], 1.0);
  dataItem->propertyVolume->ShadeOn();
  dataItem->propertyVolume->SetAmbient(0.1);
  dataItem->propertyVolume->SetDiffuse(0.9);
  dataItem->propertyVolume->SetSpecular(0.2);
  dataItem->propertyVolume->SetSpecularPower(10.0);
  dataItem->propertyVolume->SetScalarOpacityUnitDistance(0.8919);
  dataItem->propertyVolume->SetColor(dataItem->colorFunction);
  dataItem->propertyVolume->SetScalarOpacity(dataItem->opacityFunction);
  dataItem->propertyVolume->SetInterpolationTypeToLinear();
  dataItem->actorVolume->SetProperty(dataItem->propertyVolume);
  dataItem->actorVolume->SetMapper(mapperVolume.GetPointer());

  dataItem->flashlight->SwitchOff();
  dataItem->flashlight->SetLightTypeToHeadlight();
  dataItem->flashlight->SetColor(0.0, 1.0, 1.0);
  dataItem->flashlight->SetConeAngle(15);
  dataItem->flashlight->SetPositional(true);
}

//----------------------------------------------------------------------------
void ExampleVTKReader::display(GLContextData& contextData) const
{
  int numberOfSupportedClippingPlanes;
  glGetIntegerv(GL_MAX_CLIP_PLANES, &numberOfSupportedClippingPlanes);
  int clippingPlaneIndex = 0;
  for (int i = 0; i < NumberOfClippingPlanes &&
    clippingPlaneIndex < numberOfSupportedClippingPlanes; ++i)
    {
    if (ClippingPlanes[i].isActive())
      {
      /* Enable the clipping plane: */
      glEnable(GL_CLIP_PLANE0 + clippingPlaneIndex);
      GLdouble clippingPlane[4];
      for (int j = 0; j < 3; ++j)
          clippingPlane[j] = ClippingPlanes[i].getPlane().getNormal()[j];
      clippingPlane[3] = -ClippingPlanes[i].getPlane().getOffset();
      glClipPlane(GL_CLIP_PLANE0 + clippingPlaneIndex, clippingPlane);
      /* Go to the next clipping plane: */
      ++clippingPlaneIndex;
      }
    }
  /* Save OpenGL state: */
  glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|
    GL_LIGHTING_BIT|GL_POLYGON_BIT);
  /* Get context data item */
  DataItem* dataItem = contextData.retrieveDataItem<DataItem>(this);

  if(this->FlashlightSwitch[0])
    {
    dataItem->flashlight->SetPosition(this->FlashlightPosition);
    dataItem->flashlight->SetFocalPoint(this->FlashlightDirection);
    dataItem->flashlight->SwitchOn();
    }
  else
    {
    dataItem->flashlight->SwitchOff();
    }

  if (this->Outline)
    {
    dataItem->actorOutline->VisibilityOn();
    }
  else
    {
    dataItem->actorOutline->VisibilityOff();
    }
  if (this->Volume)
    {
    dataItem->actorVolume->VisibilityOn();
    dataItem->actor->VisibilityOff();
    }
  else
    {
    dataItem->actorVolume->VisibilityOff();
    dataItem->actor->GetProperty()->EdgeVisibilityOff();
    if (this->RepresentationType != -1)
      {
      dataItem->actor->VisibilityOn();
      dataItem->actor->GetProperty()->SetOpacity(this->Opacity);
      if (this->RepresentationType < 3)
        {
        dataItem->actor->GetProperty()->SetRepresentation(
          this->RepresentationType);
        }
      else if (this->RepresentationType == 3)
        {
        dataItem->actor->GetProperty()->SetRepresentationToSurface();
        dataItem->actor->GetProperty()->EdgeVisibilityOn();
        }
      }
    else
      {
      dataItem->actor->VisibilityOff();
      }
    }

  /* Render the scene */
  dataItem->externalVTKWidget->GetRenderWindow()->Render();

  glPopAttrib();

  clippingPlaneIndex = 0;
  for (int i = 0; i < NumberOfClippingPlanes &&
    clippingPlaneIndex < numberOfSupportedClippingPlanes; ++i)
    {
    if (ClippingPlanes[i].isActive())
      {
      /* Disable the clipping plane: */
      glDisable(GL_CLIP_PLANE0 + clippingPlaneIndex);
      /* Go to the next clipping plane: */
      ++clippingPlaneIndex;
      }
    }
}

//----------------------------------------------------------------------------
void ExampleVTKReader::centerDisplayCallback(Misc::CallbackData* callBackData)
{
  if(!this->DataBounds)
    {
    std::cerr << "ERROR: Data bounds not set!!" << std::endl;
    return;
    }
  Vrui::setNavigationTransformation(this->Center, this->Radius);
}

//----------------------------------------------------------------------------
void ExampleVTKReader::opacitySliderCallback(
  GLMotif::Slider::ValueChangedCallbackData* callBackData)
{
  this->Opacity = static_cast<double>(callBackData->value);
  opacityValue->setValue(callBackData->value);
}

//----------------------------------------------------------------------------
void ExampleVTKReader::changeRepresentationCallback(GLMotif::ToggleButton::ValueChangedCallbackData* callBackData)
{
    /* Adjust representation state based on which toggle button changed state: */
    if (strcmp(callBackData->toggle->getName(), "ShowSurface") == 0)
    {
      this->RepresentationType = 2;
      this->Volume = false;
    }
    else if (strcmp(callBackData->toggle->getName(), "ShowSurfaceWEdges") == 0)
    {
      this->RepresentationType = 3;
      this->Volume = false;
    }
    else if (strcmp(callBackData->toggle->getName(), "ShowWireframe") == 0)
    {
      this->RepresentationType = 1;
      this->Volume = false;
    }
    else if (strcmp(callBackData->toggle->getName(), "ShowPoints") == 0)
    {
      this->RepresentationType = 0;
      this->Volume = false;
    }
    else if (strcmp(callBackData->toggle->getName(), "ShowVolume") == 0)
    {
      this->Volume = callBackData->set;
      if (this->Volume)
        {
        this->RepresentationType = -1;
        }
    }
    else if (strcmp(callBackData->toggle->getName(), "ShowOutline") == 0)
    {
      this->Outline = callBackData->set;
    }
}
//----------------------------------------------------------------------------
void ExampleVTKReader::changeAnalysisToolsCallback(GLMotif::ToggleButton::ValueChangedCallbackData* callBackData)
{
    /* Set the new analysis tool: */
    if (strcmp(callBackData->toggle->getName(), "ClippingPlane") == 0)
    {
      this->analysisTool = 0;
    }
    else if (strcmp(callBackData->toggle->getName(), "Flashlight") == 0)
    {
      this->analysisTool = 1;
    }
    else if (strcmp(callBackData->toggle->getName(), "Other") == 0)
    {
      this->analysisTool = 2;
    }
}

//----------------------------------------------------------------------------
void ExampleVTKReader::showRenderingDialogCallback(GLMotif::ToggleButton::ValueChangedCallbackData* callBackData)
{
    /* open/close rendering dialog based on which toggle button changed state: */
  if (strcmp(callBackData->toggle->getName(), "ShowRenderingDialog") == 0) {
    if (callBackData->set) {
      /* Open the rendering dialog at the same position as the main menu: */
      Vrui::getWidgetManager()->popupPrimaryWidget(renderingDialog, Vrui::getWidgetManager()->calcWidgetTransformation(mainMenu));
    } else {
      /* Close the rendering dialog: */
      Vrui::popdownPrimaryWidget(renderingDialog);
    }
  }
}

//----------------------------------------------------------------------------
ClippingPlane * ExampleVTKReader::getClippingPlanes(void)
{
  return this->ClippingPlanes;
}

//----------------------------------------------------------------------------
int ExampleVTKReader::getNumberOfClippingPlanes(void)
{
    return this->NumberOfClippingPlanes;
}

//----------------------------------------------------------------------------
void ExampleVTKReader::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData * callbackData) {
    /* Check if the new tool is a locator tool: */
    Vrui::LocatorTool* locatorTool = dynamic_cast<Vrui::LocatorTool*> (callbackData->tool);
    if (locatorTool != 0) {
        BaseLocator* newLocator;
        if (analysisTool == 0) {
            /* Create a clipping plane locator object and associate it with the new tool: */
            newLocator = new ClippingPlaneLocator(locatorTool, this);
        }
        else if (analysisTool == 1) {
          /* Create a flashlight locator object and associate it with the new tool: */
          newLocator = new FlashlightLocator(locatorTool, this);
        }

        /* Add new locator to list: */
        baseLocators.push_back(newLocator);
    }
}

//----------------------------------------------------------------------------
void ExampleVTKReader::toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData * callbackData) {
    /* Check if the to-be-destroyed tool is a locator tool: */
    Vrui::LocatorTool* locatorTool = dynamic_cast<Vrui::LocatorTool*> (callbackData->tool);
    if (locatorTool != 0) {
        /* Find the data locator associated with the tool in the list: */
        for (BaseLocatorList::iterator blIt = baseLocators.begin(); blIt != baseLocators.end(); ++blIt) {

            if ((*blIt)->getTool() == locatorTool) {
                /* Remove the locator: */
                delete *blIt;
                baseLocators.erase(blIt);
                break;
            }
        }
    }
}

//----------------------------------------------------------------------------
int * ExampleVTKReader::getFlashlightSwitch(void)
{
  return this->FlashlightSwitch;
}

//----------------------------------------------------------------------------
double * ExampleVTKReader::getFlashlightPosition(void)
{
  return this->FlashlightPosition;
}

//----------------------------------------------------------------------------
double * ExampleVTKReader::getFlashlightDirection(void)
{
  return this->FlashlightDirection;
}
