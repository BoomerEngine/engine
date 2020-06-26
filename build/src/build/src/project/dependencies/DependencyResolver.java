package project.dependencies;

import library.Library;
import project.Project;
import utils.ProjectException;

import java.util.List;

public interface DependencyResolver {

  /* resolve library dependency */
  public Library resolveLibrary(String libraryName);

  /* resolve project dependency */
  public List<Project> resolveProject(String projectName, int maxSolutionLevel);

}
