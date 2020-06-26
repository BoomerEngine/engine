package rtti;

import org.w3c.dom.Document;
import org.w3c.dom.Node;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class RttiManifest {

    public Path manifestPath;

    public static class ProjectInfo {
        String name; // base_math
        Path sourcesDir; // Z:/projects/boomer/engine/src/base/math/src/
        Path outputDir; // Z:/projects/boomer/.temp/solution/windows.full/generated/base_math/
    };

    public List<ProjectInfo> projects = new ArrayList<>();
    public boolean includeTests = false;

    public static RttiManifest parseFromXml(Path xmlFilePath) {
        RttiManifest ret = null;
        Document doc = utils.Utils.loadXML(xmlFilePath);
        Path moduleDirectory = xmlFilePath.getParent();
        if (doc != null) {
            ret = new RttiManifest();

            try {
                Node p = doc.getDocumentElement().getFirstChild();

                for (; p != null; p = p.getNextSibling()) {
                    if (p.getNodeType() != Node.ELEMENT_NODE)
                        continue;

                    if (p.getNodeName().equals("includeTests")) {
                        ret.includeTests = true;
                    } else if (p.getNodeName().equals("project")) {
                        Node pp = p.getFirstChild();

                        ProjectInfo info = new ProjectInfo();
                        for (; pp != null; pp = pp.getNextSibling()) {
                            if (pp.getNodeType() != Node.ELEMENT_NODE)
                                continue;

                            if (pp.getNodeName().equals("output")) {
                                info.outputDir = Paths.get(pp.getTextContent());
                            } else if (pp.getNodeName().equals("src")) {
                                info.sourcesDir = Paths.get(pp.getTextContent());
                            } else if (pp.getNodeName().equals("name")) {
                                info.name = pp.getTextContent();
                            } else {
                                System.err.printf("Unexpected key '%s' in rtti manifest at '%s'\n", pp.getNodeName(), xmlFilePath);
                            }
                        }

                        if (info.sourcesDir == null || info.outputDir == null || info.name.equals("")) {
                            System.err.printf("Project definition in rtti manifest should have both sources, name and output directory defined\n");
                        } else {
                            ret.projects.add(info);
                        }
                    } else {
                        System.err.printf("Unexpected key '%s' in rtti manifest at '%s'\n", p.getNodeName(), xmlFilePath);
                    }
                }
            } catch (Exception e) {
                throw new utils.ProjectException("XML parsing error", e).location(xmlFilePath);
            }
        }

        System.out.printf("Loaded %d projects from RTTI manifest\n", ret.projects.size());
        return ret;
    }
}



