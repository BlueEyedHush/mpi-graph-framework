package perftest

import org.apache.spark.SparkContext
import org.apache.spark.graphx.{Graph, GraphLoader}
import org.apache.spark.storage.StorageLevel

object Utils {
  def loadGraphFromStdDirectory(filename: String)(implicit sc: SparkContext): Graph[Int, Int] = {
    println(s"Loading $filename")

    def pwd = System.getProperty("user.dir")
    val path = s"$pwd/../graphs/data/$filename.elt"
    GraphLoader.edgeListFile(
      sc,
      path,
      edgeStorageLevel = StorageLevel.MEMORY_AND_DISK,
      vertexStorageLevel = StorageLevel.MEMORY_AND_DISK
    )
  }
}
