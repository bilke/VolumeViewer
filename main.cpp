// STD includes
#include <iostream>
#include <string>

// ExampleVTKReader includes
#include "ExampleVTKReader.h"

void printUsage(void)
{
  std::cout << "\nExampleVTKReader - Render VTK objects in the VRUI context" << std::endl;
  std::cout << "\nUSAGE:\n\t./ExampleVTKReader [-f <string>] [-h]" << std::endl;
  std::cout << "\nWhere:" << std::endl;
  std::cout << "\t-f <string>, -fileName <string>" << std::endl;
  std::cout << "\tName of VTK file to load using VTK.\n" << std::endl;
  std::cout << "\t-r <digit>, -renderMode <digit>" << std::endl;
  std::cout << "\tRender mode to request for vtkSmartVolumeMapper.\n" << std::endl;
  std::cout << "\t-showfps" << std::endl;
  std::cout << "\tShow the FPS display by default.\n" << std::endl;
  std::cout << "\t-hidebgnotifs" << std::endl;
  std::cout << "\tHide notifications for background updates.\n" << std::endl;
  std::cout << "\t-h, -help" << std::endl;
  std::cout << "\tDisplay this usage information and exit." << std::endl;
  std::cout << "\nAdditionally, all the commandline switches the VRUI " <<
    "accepts can be passed to ExampleVTKReader.\nFor example, -rootSection," <<
    " -vruiVerbose, -vruiHelp, etc.\n" << std::endl;
}

/* Create and execute an application object: */
/*
 * main - The application main method.
 *
 * parameter argc - int
 * parameter argv - char**
 *
 */
int main(int argc, char* argv[])
{
  try
    {
    std::string name;
    bool showFPS = false;
    int renderMode = -1;
    bool verbose = false;
    bool hidebgnotifs = false;
    if(argc > 1)
      {
      /* Parse the command-line arguments */
      for(int i = 1; i < argc; ++i)
        {
        if(strcmp(argv[i], "-f")==0 || strcmp(argv[i], "-filename")==0)
          {
          name.assign(argv[i+1]);
          ++i;
          }
        if(strcmp(argv[i], "-r")==0 || strcmp(argv[i], "-renderMode")==0)
          {
          renderMode = atoi(argv[i+1]);
          ++i;
          }
        if(strcmp(argv[i], "-showfps")==0)
          {
          showFPS = true;
          }
        if(strcmp(argv[i], "-hidebgnotifs")==0)
          {
          hidebgnotifs = true;
          }
        if(strcmp(argv[i],"-h")==0 || strcmp(argv[i], "-help")==0)
          {
          printUsage();
          return 0;
          }
        if(strcmp(argv[i],"-vruiVerbose")==0)
          {
          verbose = true;
          }
        }
      }

    ExampleVTKReader application(argc, argv);
    if(!name.empty())
      {
      application.setFileName(name.c_str());
      application.setVerbose(verbose);
      }
    if(renderMode != -1)
      {
      application.setRequestedRenderMode(renderMode);
      }
    application.setShowFPS(showFPS);
    application.setProgressVisibility(!hidebgnotifs);
    application.initialize();
    application.run();
    return 0;
    }
  catch (std::runtime_error e)
    {
    std::cerr << "Error: Exception " << e.what() << "!" << std::endl;
    return 1;
    }
}
