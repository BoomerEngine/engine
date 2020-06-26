package cmake;

import generators.ProjectSourcesSetup;
import project.ProjectSources;

import java.nio.file.Path;

public class CMakeSourcesProjectSetup extends ProjectSourcesSetup {

    public CMakeSourcesProjectSetup(ProjectSources originalProject, CMakeSolutionSetup solutionSetup) {
        super(originalProject, solutionSetup);

        this.localProjectFilePath = this.localOutputPath.resolve("CMakeLists.txt");
    }

    public Path localProjectFilePath; // output/projects/<projectname>/CMakeLists.txt

}
