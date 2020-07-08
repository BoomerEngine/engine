package project;

import org.w3c.dom.Document;
import org.w3c.dom.Node;

import java.nio.file.Path;
import java.nio.file.Files;
import java.util.List;
import java.util.ArrayList;


public class ProjectManifest {

    public Path manifestPath;
    public Path projectDirectoryPath;

    public Path publishDirectoryPath;
    public Path tempDirectoryPath;
    public Path solutionDirectoryPath;

    public String name;
    public String description;
    public String author;
    public String license;

    public boolean engineProject = false;

    public String defaultSolutionType = "full";

    public List<Path> moduleAbsolutePaths = new ArrayList<>();

    public static ProjectManifest parseFromXml(Path xmlFilePath) {
        ProjectManifest ret = null;
        Document doc = utils.Utils.loadXML(xmlFilePath);
        Path moduleDirectory = xmlFilePath.getParent();
        String defaultPublishDirectory = ".bin";
        String defaultTempDirectory = ".temp";
        if (doc != null) {
            ret = new ProjectManifest();

            try {
                Node p = doc.getDocumentElement().getFirstChild();

                for (; p != null; p = p.getNextSibling()) {
                    if (p.getNodeType() != Node.ELEMENT_NODE)
                        continue;

                    if (p.getNodeName().equals("name")) {
                        ret.name = p.getTextContent();
                    } else if (p.getNodeName().equals("description")) {
                        ret.description = p.getTextContent();
                    } else if (p.getNodeName().equals("author")) {
                        ret.author = p.getTextContent();
                    } else if (p.getNodeName().equals("license")) {
                        ret.license = p.getTextContent();
                    } else if (p.getNodeName().equals("solutionType")) {
                        ret.defaultSolutionType = p.getTextContent();
                    } else if (p.getNodeName().equals("publishDir")) {
                        defaultPublishDirectory = p.getTextContent();
                    } else if (p.getNodeName().equals("engine")) {
                        ret.engineProject = true;
                    } else if (p.getNodeName().equals("tempDir")) {
                        defaultTempDirectory = p.getTextContent();
                    } else if (p.getNodeName().equals("module")) {
                        try {
                            Path modulePath = moduleDirectory.resolve(p.getTextContent()).normalize();
                            Path moduleManifestPath = modulePath.resolve("module.xml");
                            if (Files.exists(moduleManifestPath)) {
                                ret.moduleAbsolutePaths.add(modulePath);
                            } else {
                                System.err.printf("Module path '%s' specified in project manifest at '%s' does not contain a valid module (missing module.xml)\n", modulePath, xmlFilePath);
                            }
                        } catch (Exception e) {
                            System.err.printf("Invalid module path '%s' specified in project manifest at '%s': %s\n", p.getTextContent(), xmlFilePath, e.toString());
                        }
                    } else {
                        System.err.printf("Unexpected key '%s' in project manifest at '%s'\n", p.getNodeName(), xmlFilePath);
                    }
                }
            } catch (Exception e) {
                throw new utils.ProjectException("XML parsing error", e).location(xmlFilePath);
            }
        }

        if (ret.name.equals("")) {
            System.err.printf("Project manifest should contain at least the <name> field ('%s')\n", xmlFilePath);
            return null;
        }

        if (defaultTempDirectory.equals(""))
            defaultTempDirectory = ".temp";

        if (defaultPublishDirectory.equals(""))
            defaultPublishDirectory = ".bin";

        if (!defaultTempDirectory.endsWith("/") && !defaultTempDirectory.endsWith("\\"))
            defaultTempDirectory += "/";

        if (!defaultPublishDirectory.endsWith("/") && !defaultPublishDirectory.endsWith("\\"))
            defaultPublishDirectory += "/";

        ret.manifestPath = xmlFilePath;
        ret.projectDirectoryPath = moduleDirectory;
        ret.publishDirectoryPath = moduleDirectory.resolve(defaultPublishDirectory);
        ret.tempDirectoryPath = moduleDirectory.resolve(defaultTempDirectory);
        ret.solutionDirectoryPath = ret.tempDirectoryPath.resolve("solution/");
        System.out.printf("Project binaries will be written to: '%s'\n", ret.publishDirectoryPath);
        System.out.printf("Project temporary files will be written to: '%s'\n", ret.tempDirectoryPath);

        return ret;
    }
}
