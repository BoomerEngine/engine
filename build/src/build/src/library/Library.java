package library;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import utils.KeyValueTable;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;
import java.util.stream.Collectors;

public class Library {

  private static final String MANIFEST_FILE_NAME = "manifest.xml";

  public final String name;
  public final String version;
  public final boolean wellDefined;

  public final Path absolutePath; // src/lib/xxx/
  public final List<Config> configs; // may not be empty, it will at least contain the deployment information

  //--

  public Config.MergedState getSetup(String platformName, String configName) {
    Config.MergedState ret = new Config.MergedState();

    configs.stream()
            .filter(x -> x.selector.test(platformName, configName))
            .map(x -> x.state)
            .forEach(x -> ret.merge(x));

    return ret;
  }


  /*
   * load library from given directory
   */
  public Library(Path absolutePath) {
    this.name = absolutePath.getFileName().toString();
    this.absolutePath = absolutePath;
    this.version = "1";

    Document xml = utils.Utils.loadXML(getManifestPath(absolutePath));
    this.configs = toConfigList(xml);

    Element xmlExternalLib = utils.Utils.getXmlNode(xml.getDocumentElement(), "external");
    if (xmlExternalLib != null) {
      KeyValueTable params = utils.Utils.getXmlNodeAttributes(xmlExternalLib);

      String envPathName =  params.value("envpath");
      System.out.println("Found external library '" + name + "' placed at env '" + envPathName + "'");

      String envPath = System.getenv(envPathName);
      if (envPath == null || envPath.isEmpty()) {
        System.err.println("No environment variable '" + envPathName + "' specified. Library will not be used, projects using it will not compile");
        this.wellDefined = false;
      } else {
        System.out.println("External path to '" + name + "' = '" + envPath + "'");
        Path externalRootPath = Paths.get(envPath.endsWith("/") ? envPath : (envPath + "/"));
        this.configs.forEach(cfg -> cfg.resolvePaths(externalRootPath));
        this.wellDefined = true;
      }
    } else {
      System.out.println("Found general library '" + name + "' at folder '" + absolutePath + "'");
      this.configs.forEach(cfg -> cfg.resolvePaths(absolutePath));
      this.wellDefined = true;
    }
  }

  //----


  private static Path getManifestPath(Path libraryPath) {
    return libraryPath.resolve(MANIFEST_FILE_NAME);
  }

  private static List<Config> toConfigList(Document doc) {
    return utils.Utils.getXmlNodes(doc.getDocumentElement(), "setup").stream()
            .map(node -> new Config(node))
            .collect(Collectors.toList());
  }

}
