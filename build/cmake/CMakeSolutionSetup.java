package cmake;

import generators.SolutionSetup;
import project.Project;
import project.ProjectSources;
import project.Solution;
import utils.KeyValueTable;

import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.InvalidParameterException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class CMakeSolutionSetup extends SolutionSetup {

    public Path solutionPath;

    public List<CMakeSourcesProjectSetup> cmakeProjects = new ArrayList<>();
    public Map<Project, CMakeSourcesProjectSetup> cmakeProjectMap = new HashMap<>();

    //---

    public CMakeSolutionSetup(Solution originalSolution, KeyValueTable params) throws IOException {
        super(originalSolution, params);

        String rootModuleSolution = "CMakeLists.txt";

        this.solutionPath = rootOutputPath.resolve(rootModuleSolution);

        this.buildToolsPath = utils.Utils.getCurrentJar().getParentFile().toPath().normalize(); // gets the \build\tools\bin\
        this.buildScriptsPath = buildToolsPath.resolve("build/cmake/").normalize();

        this.allPlatforms.add(originalSolution.platformType.name());
        this.allConfigs.add("Debug");
        this.allConfigs.add("Checked");
        this.allConfigs.add("Release");

        System.out.printf("Build platform name: '%s'\n", originalSolution.platformType.name);
        System.out.printf("Build tools path: '%s'\n", buildToolsPath.toString());
        System.out.printf("Build scripts path: '%s'\n", buildScriptsPath.toString());

        populate();
    }

    //---

    private void populate() {
        for (Project p : originalSolution.projects) {
            if (p.enabled) {
                ProjectSources ps = (ProjectSources) p;
                if (ps != null) {
                    CMakeSourcesProjectSetup cp = new CMakeSourcesProjectSetup(ps, this);
                    projects.add(cp);
                    cmakeProjects.add(cp);
                    cmakeProjectMap.put(p, cp);
                }
            }
        }
    }

}

