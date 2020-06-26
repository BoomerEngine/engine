package library;

import org.w3c.dom.Element;
import utils.KeyValueTable;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

public class Config {

  //---

  public final MergedState state;
  public final Selector selector;

  //---

  public Config(Element xmlNode) {

    // the selector is created from the node attribute
    KeyValueTable params = utils.Utils.getXmlNodeAttributes(xmlNode);
    this.selector = Selector.createFromPattern(params.value("platform"), params.value("config"));

    // the rest is parsed
    this.state = new MergedState(xmlNode);
  }

  public void resolvePaths(Path absolutePath) {
    state.resolvePaths(absolutePath);
  }

  public static class DeployFile
  {
    public Path sourcePath;
    public String targetPath;
  }

  //---

  public static class MergedState
  {
    public List<Path> getIncludePaths() {
      return includePaths;
    }

    public List<Path> getLibPaths() {
      return libPaths;
    }

    public List<DeployFile> getDeployFiles() { return deployFiles; }

    public List<SystemLibrary> getSystemLibraries() { return systemLibraries; }

    public MergedState() {
    }

    public MergedState(Element xmlElement)
    {
      this.includePaths = toPathList(utils.Utils.getXmlNodes(xmlElement, "include"));
      this.libPaths = toPathList(utils.Utils.getXmlNodes(xmlElement, "lib"));
      this.deployFiles = toDeployFileList(utils.Utils.getXmlNodes(xmlElement, "deploy"));

      Element xmlSysystemLib = utils.Utils.getXmlNode(xmlElement, "cmake");
      if(xmlSysystemLib != null)
        this.systemLibraries.add(new SystemLibrary(xmlSysystemLib));
    }

    public void merge(MergedState c)
    {
      utils.Utils.mergeList(includePaths, c.includePaths);
      utils.Utils.mergeList(libPaths, c.libPaths);
      utils.Utils.mergeList(deployFiles, c.deployFiles);

      for (SystemLibrary sl : c.systemLibraries) {
        boolean has = false;
        for (SystemLibrary csl : systemLibraries) {
          if (csl.getName().equals(sl.getName())) {
            has = true;
            break;
          }
        }

        if (!has)
          systemLibraries.add(sl);
      }
    }

    //---

    private List<Path> includePaths = new ArrayList<>();
    private List<Path> libPaths = new ArrayList<>();
    private List<DeployFile> deployFiles = new ArrayList<>();
    private List<SystemLibrary> systemLibraries = new ArrayList<>();

    private static List<Path> toPathList(List<Element> xmlElements) {
      return xmlElements.stream()
              .map(p -> p.getTextContent())
              .filter(p -> !p.isEmpty())
              .map(p -> Paths.get(p))
              .collect(Collectors.toList());
    }

    private static List<DeployFile> toDeployFileList(List<Element> xmlElements) {
      List<DeployFile> ret = new ArrayList<>();
      for (Element e : xmlElements) {
        if (!e.getTextContent().isEmpty()) {
          DeployFile f = new DeployFile();
          f.sourcePath = Paths.get(e.getTextContent());
          f.targetPath = e.getAttribute("target");
          ret.add(f);

          if (f.targetPath.isEmpty())
            f.targetPath = f.sourcePath.getFileName().toString();
        }
      }
      return ret;
    }

    public void resolvePaths(Path absolutePath) {
      includePaths = resolvePaths(includePaths, absolutePath);
      libPaths = resolvePaths(libPaths, absolutePath);
      deployFiles = resolveDeployPaths(deployFiles, absolutePath);
    }

    private static List<Path> resolvePaths(List<Path> deployPaths, Path absolutePath) {
      return deployPaths.stream()
              .map(path -> absolutePath.resolve(path))
              .collect(Collectors.toList());
    }

    private static List<DeployFile> resolveDeployPaths(List<DeployFile> deployPaths, Path absolutePath) {
      for (DeployFile f : deployPaths)
        f.sourcePath = absolutePath.resolve(f.sourcePath);
      return deployPaths;
    }
  }

  //---

}
