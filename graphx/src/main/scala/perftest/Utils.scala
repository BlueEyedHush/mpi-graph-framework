package perftest

import org.apache.spark.SparkContext
import org.apache.spark.graphx.{Graph, GraphLoader}
import org.apache.spark.storage.StorageLevel

object Utils {
  def stdGraphNameToPath(filename: String): String = {
    def pwd = System.getProperty("user.dir")
    s"$pwd/../graphs/data/$filename.elt"
  }

  def relGraphPathToPath(relPath: String): String = {
    def pwd = System.getProperty("user.dir")
    s"$pwd/$relPath"
  }

  def loadGraph(path: String)(implicit sc: SparkContext): Graph[Int, Int] = {
    println(s"Loading $path")
    GraphLoader.edgeListFile(
      sc,
      path,
      edgeStorageLevel = StorageLevel.MEMORY_AND_DISK,
      vertexStorageLevel = StorageLevel.MEMORY_AND_DISK
    )
  }
}
