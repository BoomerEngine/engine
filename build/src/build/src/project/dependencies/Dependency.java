package project.dependencies;

import generators.PlatformType;
import library.Library;
import project.Project;
import utils.ProjectException;

import java.security.InvalidParameterException;
import java.util.ArrayList;
import java.util.List;

public class Dependency {

  public final DependencyType type;
  public final Project reporter;
  public final String target;

  public List<Project> resolvedProjects = new ArrayList<>();
  public  Library resolvedLibrary = null;

  private boolean hardDependency = true;
  private boolean validDependency = true;

  //---

  public Dependency(Project reporter, DependencyType type, String target) {
    this.hardDependency = !target.endsWith("*");
    this.reporter = reporter;
    this.target = target;
    this.type = type;
  }

  //--

  public boolean library() {
    return (type == DependencyType.PUBLIC_LIBRARY) || (type == DependencyType.PRIVATE_LIBRARY) || (type == DependencyType.PLATFORM_LIBRARY);
  }

  public void resolveDependency(DependencyResolver resolver) {
    if (library()) {
      resolvedLibrary = resolver.resolveLibrary(target);
      if (resolvedLibrary == null) {
        System.err.printf("Library '%s' used by project '%s' was not found.\n", target, reporter.mergedName);
        reporter.excludeFromBuild();
        validDependency = false;
      }
    } else {
      int minmalLevel = reporter.attributes.hasKey("app") ? 10 : reporter.module.dependencyLevel; // allow apps to have general dependency list
      List<Project> ret = resolver.resolveProject(target, minmalLevel);
      if (ret == null) {
        System.err.printf("Unable to resolve dependency of project '%s' on project '%s'. Target project was not found.\n", reporter.mergedName, target);
        validDependency = false;
        if (hardDependency)
          reporter.excludeFromBuild();
      } else {
        resolvedProjects = ret;
      }
    }
  }

  public void validateDependency() {
    if (!library()) {
      List<Project> newList = new ArrayList<>();
      for (Project p : resolvedProjects) {
        if (!p.enabled) {
          if (hardDependency) {
            if (reporter.attributes.hasKey("devonly") == p.attributes.hasKey("devonly")) {
              // we have a failed project... we are invalid as well, exclude the project that uses this dependency
              System.err.printf("Dependent project '%s' used by project '%s' got disabled. Removing from dependency list\n", p.mergedName, reporter.mergedName);
              reporter.excludeFromBuild();
              validDependency = false;
            } else {
              System.err.printf("Dependent project '%s' used by project '%s' got disabled but we can live without it\n", p.mergedName, reporter.mergedName);
            }
          }
        } else {
          newList.add(p);
        }
      }
      resolvedProjects = newList;
    }
  }



}
