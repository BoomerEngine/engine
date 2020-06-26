package project;

import generators.ProjectSourcesSetup;
import project.dependencies.Dependency;
import project.dependencies.DependencyType;
import utils.KeyValueTable;
import utils.ProjectException;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

public class Project {

  public Group group;
  public Solution solution;
  public Module module;
  public ProjectType type;

  public String shortName; // baseTest
  public String mergedName; // base_test
  public Path absolutePath; // Z:\git\boomer\engine\src\base\test\

  public KeyValueTable attributes;

  public List<Dependency> dependencies = new ArrayList<>();

  public boolean enabled = true;

  //--

  Project(Solution sol, Group group, Path absoluteDir, Module module, ProjectType type) {
    this.type = type;
    this.solution = sol;
    this.module = module;
    this.group = group;
    this.shortName = absoluteDir.getFileName().toString();
    if (group != null)
      this.mergedName = group.mergedName + "_" + this.shortName;
    else
      this.mergedName = this.shortName;
    this.absolutePath = absoluteDir.normalize();
  }

  public void excludeFromBuild() {
    if (enabled){
      System.out.println(String.format("Excluding project %s from solution generation", mergedName));
      enabled = false;
    }
  }

  //----





}
