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
#include <vtkCubeSource.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractVOI.h>
#include <vtkGenericDataObjectReader.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkLight.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkStructuredGridGeometryFilter.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredPoints.h>
#include <vtkUnstructuredGrid.h>
#include <vtkRectilinearGrid.h>

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
  this->flashlight = vtkSmartPointer<vtkLight>::New();
  this->flashlight->SwitchOff();
  this->flashlight->SetLightTypeToHeadlight();
  this->flashlight->SetColor(0.0, 1.0, 1.0);
  this->flashlight->SetConeAngle(15);
  this->flashlight->SetPositional(true);
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
  renderingDialog(NULL),
  RepresentationType(2),
  Verbose(false)
{
  /* Create the user interface: */
  renderingDialog = createRenderingDialog();
  mainMenu=createMainMenu();
  Vrui::setMainMenu(mainMenu);

  this->DataDimensions = new int[3];
  this->DataBounds = new double[6];
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
  if(this->DataBounds)
    {
    delete[] this->DataBounds;
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

  GLMotif::RadioBox* representation_RadioBox = new GLMotif::RadioBox("Representation RadioBox",representationMenu,true);

  GLMotif::ToggleButton* showSurface=new GLMotif::ToggleButton("ShowSurface",representation_RadioBox,"Surface");
  showSurface->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeRepresentationCallback);
  GLMotif::ToggleButton* showWireframe=new GLMotif::ToggleButton("ShowWireframe",representation_RadioBox,"Wireframe");
  showWireframe->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeRepresentationCallback);
  GLMotif::ToggleButton* showPoints=new GLMotif::ToggleButton("ShowPoints",representation_RadioBox,"Points");
  showPoints->getValueChangedCallbacks().add(this,&ExampleVTKReader::changeRepresentationCallback);

  representation_RadioBox->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
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

  if(this->FileName)
    {
    vtkNew<vtkGenericDataObjectReader> reader;
    reader->SetFileName(this->FileName);
    reader->Update();
    if(reader->IsFilePolyData())
      {
      if(this->Verbose)
        {
        std::cout << std::endl << "ExampleVTKReader: File " << this->FileName <<
          " is of type vtkPolyData" << std::endl;
        }
      vtkNew<vtkPolyDataMapper> mapper;
      dataItem->actor->SetMapper(mapper.GetPointer());
      mapper->SetInputData(reader->GetPolyDataOutput());
      reader->GetPolyDataOutput()->GetBounds(this->DataBounds);
      }
    else if(reader->IsFileStructuredGrid())
      {
      if(this->Verbose)
        {
        std::cout << std::endl << "ExampleVTKReader: File " << this->FileName <<
          " is of type vtkStructuredGrid" << std::endl;
        }
      vtkNew<vtkPolyDataMapper> mapper;
      dataItem->actor->SetMapper(mapper.GetPointer());
      vtkNew<vtkStructuredGridGeometryFilter> geometryFilter;
      geometryFilter->SetInputData(reader->GetStructuredGridOutput());
      geometryFilter->Update();
      geometryFilter->GetOutput()->GetBounds(this->DataBounds);
      mapper->SetInputData(geometryFilter->GetOutput());
      }
    else if(reader->IsFileStructuredPoints())
      {
      if(this->Verbose)
        {
        std::cout << std::endl <<  "ExampleVTKReader: File " << this->FileName <<
          " is of type vtkStructuredPoints" << std::endl;
        }
      reader->GetStructuredPointsOutput()->GetDimensions(this->DataDimensions);
      vtkNew<vtkExtractVOI> extractVOI;
      extractVOI->SetVOI(0, this->DataDimensions[0], 0, this->DataDimensions[1], 0, this->DataDimensions[1]);
      extractVOI->SetSampleRate(1, 1, 1);
      extractVOI->SetInputData(reader->GetStructuredPointsOutput());
      extractVOI->ReleaseDataFlagOff();
      vtkNew<vtkPolyDataMapper> mapper;
      dataItem->actor->SetMapper(mapper.GetPointer());
      vtkNew<vtkImageDataGeometryFilter> geometryFilter;
      geometryFilter->SetInputConnection(extractVOI->GetOutputPort());
      geometryFilter->Update();
      geometryFilter->GetOutput()->GetBounds(this->DataBounds);
      mapper->SetInputData(geometryFilter->GetOutput());
      }
    else if(reader->IsFileUnstructuredGrid())
      {
      if(this->Verbose)
        {
        std::cout << std::endl << "ExampleVTKReader: File " << this->FileName <<
          " is of type vtkUnstructuredGrid" << std::endl;
        }
      vtkNew<vtkDataSetMapper> mapper;
      dataItem->actor->SetMapper(mapper.GetPointer());
      mapper->SetInputData(reader->GetUnstructuredGridOutput());
      reader->GetUnstructuredGridOutput()->GetBounds(this->DataBounds);
      }
    else if(reader->IsFileRectilinearGrid())
      {
      if(this->Verbose)
        {
        std::cout << std::endl << "ExampleVTKReader: File " << this->FileName <<
          " is of type vtkRectilinearGrid" << std::endl;
        }
      vtkNew<vtkDataSetMapper> mapper;
      dataItem->actor->SetMapper(mapper.GetPointer());
      mapper->SetInputData(reader->GetRectilinearGridOutput());
      reader->GetRectilinearGridOutput()->GetBounds(this->DataBounds);
      }
    }
  else
    {
    vtkNew<vtkPolyDataMapper> mapper;
    dataItem->actor->SetMapper(mapper.GetPointer());
    vtkNew<vtkCubeSource> cube;
    cube->Update();
    cube->GetOutput()->GetBounds(this->DataBounds);
    mapper->SetInputData(cube->GetOutput());
    }
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

  /* Set actor opacity */
  dataItem->actor->GetProperty()->SetOpacity(this->Opacity);
  dataItem->actor->GetProperty()->SetRepresentation(this->RepresentationType);
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
    }
    else if (strcmp(callBackData->toggle->getName(), "ShowWireframe") == 0)
    {
      this->RepresentationType = 1;
    }
    else if (strcmp(callBackData->toggle->getName(), "ShowPoints") == 0)
    {
      this->RepresentationType = 0;
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
