package rtti;

import com.google.common.base.Stopwatch;
import generators.PlatformType;
import generators.SolutionType;
import project.ProjectManifest;
import project.Solution;
import project.SolutionGeneratorCommand;
import utils.*;

import java.nio.charset.Charset;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;

public class RttiGeneratorCommand {

    public static Path DetermineRTTIManifestPath(ProjectManifest project, KeyValueTable options) {
        if (options.hasKey("manifest")) {
            return Paths.get(options.value("manifest"));
        } else {
            // use convoluted way, may not work...
            // determine where is the directory with solution
            PlatformType platformType = SolutionGeneratorCommand.DeterminePlatform(options);
            SolutionType solutionType = SolutionGeneratorCommand.DetermineSolutionType(project, options);
            String generatorName = SolutionGeneratorCommand.DetermineGeneratorName(project, options);

            // determine where the solution files should be located
            Path buildSolutionOutputPath = SolutionGeneratorCommand.DetermineOutputFolder(project.solutionDirectoryPath, platformType, solutionType, generatorName);
            return buildSolutionOutputPath.resolve("rtti.xml");
        }
    }

    public static void GenerateRTTI(ProjectManifest project, KeyValueTable options) {
        // get path to manifest file
        Path manifestPath = DetermineRTTIManifestPath(project, options);
        if (!Files.exists(manifestPath)) {
            System.err.printf("RTTI manifest not found at '%s'\n", manifestPath);
            System.exit(1);
        }

        // load it
        System.out.printf("RTTI manifest found at '%s'\n", manifestPath);
        RttiManifest manifest = RttiManifest.parseFromXml(manifestPath);
        if (manifest == null) {
            System.err.printf("RTTI manifest at '%s' failed to parse correctly\n", manifestPath);
            System.exit(1);
        }

        // create a RTTI generator for every referenced project
        List<DocGenerator> generators = new ArrayList<DocGenerator>();
        for (RttiManifest.ProjectInfo p : manifest.projects)
            generators.add(new DocGenerator(p.sourcesDir, p.outputDir, p.name, manifest.includeTests));

        // generate the rtti on each generator
        GeneratedFilesCollection files = new GeneratedFilesCollection();
        generators.parallelStream().forEach(g -> g.processAndGenerateReflection(files));

        // count final stats
        int totalFiles = 0;
        int totalTypes = 0;
        for (DocGenerator g : generators) {
            totalFiles += g.numFiles;
            totalTypes += g.numTypes;
        }
        System.out.printf("Processed %d file(s), extracted information about %d type(s)\n", totalFiles, totalTypes);

        // save files
        int numSaved = files.save(false);
        System.out.println(String.format("Saved %d files, %d were up to date out of %d total", numSaved, files.count() - numSaved, files.count()));
    }
}
