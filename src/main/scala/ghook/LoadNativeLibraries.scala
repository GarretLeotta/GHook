package ghook

import java.util.UUID
import java.io.{FileOutputStream, File}
import java.nio.channels.Channels

/** TODO: sbt-jni doesn't really work with windows currently, probably need to update scala, jdk
 *  sbt-jni has the below functionality built in
 *  this PR adds windows support to sbt-jni: https://github.com/jodersky/sbt-jni/pull/27
 *
 * NOTE: this code taken from https://github.com/davidmweber/scopus
 */
object LoadNativeLibraries {
  private val tempPath = "ghook_" + UUID.randomUUID.toString.split("-").last
  private val path     = "native"
  private val destDir  = System.getProperty("java.io.tmpdir") + "/" + tempPath + "/"

  new File(destDir).mkdirs()

  def loadLib(libName: String, load: Boolean = true): Unit = {
    try {
      val source  = Channels.newChannel(LoadNativeLibraries.getClass.getClassLoader.getResourceAsStream(path + "/" + libName))
      val fileOut = new File(destDir, libName)
      val dest    = new FileOutputStream(fileOut)
      dest.getChannel.transferFrom(source, 0, Long.MaxValue)
      source.close()
      dest.close()

      // Tee up deletion of the temporary library file when we quit
      sys.addShutdownHook {
        new File(destDir, libName).delete
        // Attempt to delete the directory also. It goes if the directory is empty
        new File(destDir).delete
      }
      // Finally, load the dynamic library if required
      if (load) System.load(fileOut.getAbsolutePath)
    } catch {
      // This is pretty catastrophic so bail.
      case e: Exception =>
        println(s"Fatal error in LoadNativeLibraries while loading $path/$libName: ${e.getMessage}")
        sys.exit(-1)
    }
  }

  def apply(): Unit = {
    loadLib("libjni_ghook.dll")
  }
}
