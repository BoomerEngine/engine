package project;

import generators.PlatformType;
import library.Library;
import project.dependencies.Dependency;
import project.dependencies.DependencyResolver;
import project.dependencies.DependencyType;
import utils.ProjectException;

import java.nio.file.Path;
import java.util.*;
import java.util.stream.Collectors;

public class ModuleRegistry {

  //---

  public ModuleRegistry() {
  }

  public List<Module> modules = new ArrayList<>();
  public List<Path> sourceRootPaths = new ArrayList<>();
  public List<Path> dataRootPaths = new ArrayList<>();
  public Set<ModuleManifest.ThirtPartyPackage> thirdPartyPackages = new HashSet<>();

  public void createModuleFromPath(Path moduleAbsoluteDirectoryPath, int dependencyLevel) {
    // load manifest, if it fails we just skip the module
    Path manifestPath = moduleAbsoluteDirectoryPath.resolve("module.xml").normalize();
    ModuleManifest manifest = ModuleManifest.parseFromXml(manifestPath);
    if (manifest == null) {
      System.err.printf("ERROR: Failed to load manifest for module from '%s' it will be skipped - project may not compile", moduleAbsoluteDirectoryPath);
      return;
    }

    // do we already have module with that name ?
    for (Module m : modules) {
      if (m.manifest.name.equals(manifest.name)) {
        System.err.printf("Module '%s' already registered from '%s'. Second copy at '%s' ignored\n", manifest.name, m.rootDirectory, moduleAbsoluteDirectoryPath);
        return;
      }
    }

    // register
    System.out.printf("Found module '%s' at '%s'\n", manifest.name, moduleAbsoluteDirectoryPath);
    Module module = new Module(moduleAbsoluteDirectoryPath, manifest, dependencyLevel);
    modules.add(module);

    // add source dir
    if (module.sourceDirectory != null)
      sourceRootPaths.add(module.sourceDirectory);

    // add data dir
    if (module.dataDirectory != null)
      dataRootPaths.add(module.dataDirectory);

    // extract all third party sources
    for (ModuleManifest.ThirtPartyPackage link : manifest.thirdPartyPackages)
      thirdPartyPackages.add(link);
  }

  //---



}
