package project;

import com.google.common.collect.Lists;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import utils.KeyValueTable;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;
import java.nio.file.*;
import java.util.ArrayList;


public class ModuleManifest {

    public String name;
    public String description;
    public String author;
    public String license;

    public static class ThirtPartyPackage
    {
        public String downloadLink;
        public String name;
    }

    public List<ThirtPartyPackage> thirdPartyPackages = new ArrayList<>();

    public static ModuleManifest parseFromXml(Path xmlFilePath) {
        ModuleManifest ret = null;

        if (Files.exists(xmlFilePath)) {
            Document doc = utils.Utils.loadXML(xmlFilePath);
            if (doc != null) {
                ret = new ModuleManifest();

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
                        } else if (p.getNodeName().equals("thirdparty")) {
                            Node name = p.getAttributes().getNamedItem("name");
                            Node path = p.getAttributes().getNamedItem("path");
                            if (name != null && path != null) {
                                ThirtPartyPackage packageInfo = new ThirtPartyPackage();
                                packageInfo.name = name.getTextContent();
                                packageInfo.downloadLink = path.getTextContent();
                                ret.thirdPartyPackages.add(packageInfo);
                            }
                        } else {
                            System.err.printf("Unexpected key '%s' in module manifest at '%s'\n", p.getNodeName(), xmlFilePath);
                        }
                    }

                    if (ret.name.equals("")) {
                        System.err.printf("Module manifest should contain at least the <name> field ('%s')\n", xmlFilePath);
                        ret = null;
                    }
                } catch (Exception e) {
                    throw new utils.ProjectException(String.format("XML parsing error: %s", e.toString()), e).location(xmlFilePath);
                }
            } else {
                System.err.printf("Failed to parse module manifest at '%s'\n", xmlFilePath);
            }
        } else {
            System.err.printf("Module manifest at '%s' is missing\n", xmlFilePath);
        }

        return ret;
    }
}
