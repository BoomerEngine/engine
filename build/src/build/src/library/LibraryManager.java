package library;

import project.ModuleManifest;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.Files;
import java.io.File;
import java.util.*;
import java.util.stream.Collectors;

public class LibraryManager {

    public List<Library> libraries = new ArrayList<>(); // libraries imported from the module
    public List<Path> librariesDirectory = new ArrayList<>();

    private Path libraryDownloadRootPath;
    private Path libraryInstallRootPath;
    private Set<String> collectedPackages = new HashSet<>();

    public LibraryManager(Path libraryDownloadRootPath, Path libraryInstallRootPath) {
        this.libraryDownloadRootPath = libraryDownloadRootPath;
        this.libraryInstallRootPath = libraryInstallRootPath;
    }

    // find library by name
    public Library libraryByName(String name) {
        for (Library l : libraries)
            if (l.name.equals(name))
                return l;

        return null;
    }

    // find a file somewhere in the libraries (hacky and slow)
    public Path resolveFileInLibraries(String relativePath) {
        for (Path p : librariesDirectory) {
            Path resolvedPath = p.resolve(relativePath);
            if (resolvedPath != null && Files.exists(resolvedPath))
                return resolvedPath;
        }

        return null;
    }

    // install remote package with libraries
    public void addRemoteLibraryPackage(ModuleManifest.ThirtPartyPackage url) {
        // make sure we only install it once
        if (collectedPackages.contains(url.name))
            return;
        collectedPackages.add(url.name);

        // check if already installed
        Path libraryDirectory = libraryInstallRootPath.resolve(url.name);
        if (!Files.exists(libraryDirectory)) {
            // download, check if downloaded
            Path zipPath = libraryDownloadRootPath.resolve(String.format("%s.zip", url.name));
            if (!Files.exists(zipPath)) {
                System.out.printf("Third party library package '%s' will be downloaded.\n", url.name);
                if (!LibraryDownloader.DownloadFile(url.downloadLink, zipPath, null)) {
                    System.err.printf("Failed to download third party library package '%s' - project may not compile.\n", url.name);
                    return;
                }
            }

            // unpack
            try {
                LibraryDownloader.UnzipFile(zipPath.toString(), libraryDirectory.toString());
                System.out.printf("Third party library package '%s' extracted and installed at '%s'.\n", url.name, libraryDirectory);
            } catch (Exception e) {
                System.err.printf("Third party library package '%s' failed to extract into '%s'.\n", url.name, libraryDirectory);
                return;
            }
        } else {
            System.out.printf("Third party library package '%s' already installed at '%s'.\n", url.name, libraryDirectory);
        }

        // now, scan the directory for libraries
        if (Files.exists(libraryDirectory)) {
            librariesDirectory.add(libraryDirectory);
            for (Path subDirPath : utils.Utils.getSubDirPaths(libraryDirectory)) {
                if (IsLibraryPath(subDirPath)) {
                    libraries.add(new Library(subDirPath));
                }
            }
        }
    }

    private static boolean IsLibraryPath(Path absolutePath) {
        Path manifestPath = absolutePath.resolve("manifest.xml");
        return Files.exists(manifestPath);
    }

    private String parseLibraryPackageNameFromURL(String url) {
        // http://www.mediafire.com/file/dpucq9ts2kyd2x9/thirdparty_engine_20_06.zip/file

        int start = url.indexOf("thirdparty_");
        int end = url.indexOf(".zip");
        if (start != -1 && end != -1)
            return String.format("lib_%s", url.subSequence(start+11, end));

        return String.format("lib_%08x", url.toLowerCase().hashCode());
    }


}
