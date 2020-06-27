package library;

import com.google.common.base.Stopwatch;

import java.io.*;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.net.HttpURLConnection;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * A utility that downloads a file from a URL.
 * @author www.codejava.net
 *
 */
public class LibraryDownloader {
  private static final int BUFFER_SIZE = 1024 * 1024;

  private static int CopyStreamContent(InputStream i, OutputStream o) throws java.io.IOException {
    int bytesRead = -1;
    int totalSize = 0;
    byte[] buffer = new byte[BUFFER_SIZE];
    while ((bytesRead = i.read(buffer)) != -1) {
      o.write(buffer, 0, bytesRead);
      totalSize += bytesRead;
    }
    o.close();
    i.close();
    return totalSize;
  }

  private static List<String> ParseCookies(HttpURLConnection conn) {
    List<String> ret = new ArrayList<>();

    for (int i = 0;; i++) {
      String headerName = conn.getHeaderFieldKey(i);
      String headerValue = conn.getHeaderField(i);

      if (headerName == null && headerValue == null)
        break;

      if ("Set-Cookie".equalsIgnoreCase(headerName)) {
        ret.add(headerValue);
        /*String[] fields = headerValue.split(";\\s*");
        for (int j = 1; j < fields.length; j++) {
          if ("secure".equalsIgnoreCase(fields[j])) {
            System.out.println("secure=true");
          } else if (fields[j].indexOf('=') > 0) {
            String[] f = fields[j].split("=");
            if ("expires".equalsIgnoreCase(f[0])) {
              System.out.println("expires"+ f[1]);
            } else if ("domain".equalsIgnoreCase(f[0])) {
              System.out.println("domain"+ f[1]);
            } else if ("path".equalsIgnoreCase(f[0])) {
              System.out.println("path"+ f[1]);
            }
          }
        }*/
      }
    }

    return ret;
  }


  public static boolean DownloadFile(String fileURL, Path saveFilePath, List<String> cookies) {
    boolean ret = false;

    try {
      Stopwatch timer = Stopwatch.createStarted();
      URL url = new URL(fileURL);
      HttpURLConnection httpConn = (HttpURLConnection) url.openConnection();

      if (cookies != null) {
        String fullCookie = "";
        for (String cookie : cookies) {
          if (!fullCookie.isEmpty())
            fullCookie += "; ";
          fullCookie += cookie;
        }
        httpConn.setRequestProperty("Cookie", fullCookie);
      }

      int responseCode = 0;
      try {
        responseCode = httpConn.getResponseCode();
      } catch (java.net.ProtocolException e) {
        System.err.println("Exception during download of '" + saveFilePath + "' :" + e.toString());
        return false;
      }

      // always check HTTP response code first
      if (responseCode == HttpURLConnection.HTTP_OK) {
        String disposition = httpConn.getHeaderField("Content-Disposition");
        String contentType = httpConn.getContentType();
        int contentLength = httpConn.getContentLength();

        // GoogleDrive AV bullshit
        if (contentType.startsWith("text/html")) {
          // we already have the "confirm", something failed
          if (fileURL.contains("confirm=")) {
            System.err.printf("Failed to by-pass GoogleDrive AV webpage, please download '%s' manually and place it in '%s'\n", fileURL, saveFilePath);
            return false;
          }

          // opens an output stream to save into file
          ByteArrayOutputStream outputStream = new ByteArrayOutputStream(65536);
          CopyStreamContent(httpConn.getInputStream(), outputStream);

          List<String> newCookies = ParseCookies(httpConn);

          String str = new String(outputStream.toByteArray());
          int loc = str.indexOf(";confirm=");
          if (loc == -1) {
            System.err.printf("Failed to get confirmation code from GoogleDrive AV page for '%s'\n", fileURL);
            return false;
          }

          String conf = str.substring(loc + 9, loc + 9 + 4);
          System.out.printf("Cofirmation code found: '%s'\n", conf);

          int idPos = fileURL.indexOf("&id=");
          String id = fileURL.substring(idPos + 4);
          String head = fileURL.substring(0, idPos);

          String fullPath = head + "&confirm=" + conf + "&id=" + id;
          ret = DownloadFile(fullPath, saveFilePath, newCookies);
        } else if (contentType.equals("application/x-zip-compressed") || contentType.equals("application/zip")) {
          // create target folder
          {
            File f = new File(saveFilePath.toString());
            f.getParentFile().mkdirs();
          }

          // opens an output stream to save into file
          FileOutputStream outputStream = new FileOutputStream(saveFilePath.toString());
          int totalSize = CopyStreamContent(httpConn.getInputStream(), outputStream);

          ret = true;
          Double f = timer.stop().elapsed(TimeUnit.MILLISECONDS) / 1000.0;
          System.out.printf("Library '%s' downloaded in %f s (%f MB/s)\n", fileURL, f, (totalSize / f) / 1000000.0);
        } else {
          System.err.printf("Expected compressed zip content, got '%s'\n", contentType);
        }
      } else {
        System.err.println("No file to download. Server replied HTTP code: " + responseCode);
      }
      httpConn.disconnect();
    } catch (Exception e) {
      System.err.println("Exception during download of '" + saveFilePath + "' :" + e.toString());
    }

    return ret;
  }

  public static void MakePath(File path) {
    File parent = path.getParentFile();
    if (parent != null)
      MakePath(parent);
    path.mkdir();
  }

  public static void UnzipFile(String zipFilePath, String destDirectory) throws IOException {
    File destDir = new File(destDirectory);
    MakePath(destDir);

    ZipInputStream zipIn = new ZipInputStream(new FileInputStream(zipFilePath));
    ZipEntry entry = zipIn.getNextEntry();
    // iterates over entries in the zip file
    while (entry != null) {
      String filePath = destDirectory + File.separator + entry.getName();
      if (!entry.isDirectory()) {
        // if the entry is a file, extracts it
        ExtractFile(zipIn, filePath);
      } else {
        // if the entry is a directory, make the directory
        File dir = new File(filePath);
        dir.mkdir();
      }
      zipIn.closeEntry();
      entry = zipIn.getNextEntry();
    }
    zipIn.close();
  }

  static private void ExtractFile(ZipInputStream zipIn, String filePath) throws IOException {
    BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(filePath));
    System.out.printf("Extracting '%s'\n", filePath);
    byte[] bytesIn = new byte[BUFFER_SIZE];
    int read = 0;
    while ((read = zipIn.read(bytesIn)) != -1) {
      bos.write(bytesIn, 0, read);
    }
    bos.close();

  }
}