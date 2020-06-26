import com.google.common.base.Stopwatch;
import generators.SolutionType;
import project.ProjectManifest;
import project.SolutionGeneratorCommand;
import rtti.RttiGeneratorCommand;
import utils.KeyValueTable;
import utils.ProjectException;

import java.nio.file.Paths;
import java.nio.file.Path;
import java.nio.file.Files;
import java.util.*;
import java.util.stream.Collectors;

public class Main {

  public static void help() {
    System.out.println("");
    System.out.println("Usage:");
    System.out.println("  java -jar build.jar -job=<command> -project=<project.xml> [options]");
    System.out.println("");
    System.out.println("Main commands:");
    System.out.println("  generate - Generate solution for project sources so it can be compiled (DEFAULT)");
    System.out.println("  publish - Publish project binaries and data into a standalone build");
    System.out.println("");
    System.out.println("Additional options:");
    System.out.println("  -solution=<full|project|final> - override solution type specified in the project");
    System.out.println("  -generator=<name> - override solution generator, by default we guess it based on the platform");
    System.out.println("");
  }

    public static void main(String[] args) {
      // title bar
      System.out.println("BoomerEngine Code Generator v1.0");

      // enter protected block
      try {
        // parse commandline
        KeyValueTable commandline = KeyValueTable.parseFromCommandline(args);

        // get the command to run
        String command = commandline.valueOrDefault("command","generate");
        String projectFile = commandline.value("project");

        // if project file was not specified try to guess it
        if (projectFile.equals("")) {
          Path currentWorkingDir = Paths.get("").toAbsolutePath();
          System.out.printf("Project was not specified, trying to find project.xml at '%s'\n", currentWorkingDir);

          Path testProjectFile = currentWorkingDir.resolve("project.xml");
          if (!Files.exists(testProjectFile)) {
            System.err.printf("No project file specified in commandline and no project.xml found in current directory (%s), nothing to build :(\n", currentWorkingDir);
            System.exit(1);
            return;
          }

          projectFile = testProjectFile.toString();
        }

        // validate manifest path is ok
        Path projectMainPath = Paths.get(projectFile);
        if (!Files.exists(projectMainPath)) {
          System.err.printf("Project file '%s' specified in the commandline does not exist, nothing to build\n", projectFile);
          System.exit(1);
        }

        // load project manifest
        ProjectManifest pm = ProjectManifest.parseFromXml(projectMainPath);
        if (null == pm) {
          System.err.printf("Project manifest '%s' is invalid or missing, nothing to build\n", projectMainPath);
          System.exit(1);
        }

        // run the job
        Stopwatch timer = Stopwatch.createStarted();
        if (command.equals("generate")) {
          //SolutionType solutionType = DetermineSolutionType(project, options);
          SolutionGeneratorCommand.GenerateSolution(pm, commandline, SolutionType.FULL);
          SolutionGeneratorCommand.GenerateSolution(pm, commandline, SolutionType.FINAL);
          RttiGeneratorCommand.GenerateRTTI(pm, commandline);
        } else if (command.equals("rtti")) {
          RttiGeneratorCommand.GenerateRTTI(pm, commandline);
        } else {
          System.err.printf("Unknown command '%s'\n", command);
          System.exit(1);
        }
        System.out.printf("Command '%s' finished in %s\n", command, timer.stop());
      } catch (ProjectException e) {
        e.printStackTrace();
        System.exit(1); // user caused exit code
      } catch (RuntimeException e) {
        e.printStackTrace();
        System.exit(-2); // internal crash exit code
      } catch (Exception e) {
        e.printStackTrace();
        System.exit(-3); // internal crash exit code
      }
    }
}
