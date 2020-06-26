package project;

import generators.PlatformType;
import library.Library;
import library.LibraryManager;
import project.dependencies.Dependency;
import project.dependencies.DependencyResolver;
import project.dependencies.DependencyType;

import java.nio.file.Path;
import java.util.*;
import java.util.stream.Collectors;

public class SolutionDependencyResolver implements DependencyResolver {

  //---

  private LibraryManager libs;
  private Solution solution;

  public SolutionDependencyResolver(Solution sol, LibraryManager libs) {
    this.solution = sol;
    this.libs = libs;
  }

  @Override
  public Library resolveLibrary(String libraryName) {
      return libs.libraryByName(libraryName);
  }

  @Override
  public List<Project> resolveProject(String projectName, int maxSolutionLevel) {
    if (projectName.equals("*all_tests*")) {
      return solution.projects.stream()
              .filter(proj -> (proj.attributes.hasKey("hasTests") && proj.module.dependencyLevel == maxSolutionLevel))
              .collect(Collectors.toList());
    }
    else if (projectName.endsWith("*")) {
      String stem = projectName.substring(0, projectName.length()-1);
      return solution.projects.stream()
              .filter(proj -> proj.mergedName.startsWith(stem))
              .filter(proj -> proj.module.dependencyLevel <= maxSolutionLevel)
              .filter(proj -> !proj.attributes.hasKey("app")) // don't allow apps as dependencies
              //.peek(proj -> {System.out.println(String.format("Matched pattern '%s' with project '%s'", stem, proj.mergedName));})
              .collect(Collectors.toList());
    } else {
      Project p = solution.projectByName(projectName);
      if (p == null) {
        System.err.printf("Unable to resolve reference to project '" + projectName + "' because project does not exist\n");
        return null;
      }
      else if (p.module.dependencyLevel > maxSolutionLevel) {
        System.err.printf("Cannot use project '" + projectName + "' from here because it's part of different solution tier (ie. you can import project shit inside base code, etc)\n");
        return null;
      }

      List<Project> ret = new ArrayList<>();
      ret.add(p);
      return ret;
    }
  }

}
