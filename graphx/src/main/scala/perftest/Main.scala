package perftest

import org.apache.log4j.{Level, Logger}
import org.apache.spark._
import org.apache.spark.graphx._
import org.apache.spark.storage.StorageLevel

import scala.io.Source

object Main {
  // Start Spark.
  def run(implicit sc: SparkContext) = {
    // Suppress unnecessary logging.
    Logger.getRootLogger.setLevel(Level.ERROR)

    val g: Graph[Boolean, Int] = Utils.loadGraphFromStdDirectory("powerlaw_25_2_05_876")
      .mapVertices((vid, _) => false)

    // Calculate centralities.
    println("\n### Bfs\n")
    val (startVertexId, _) = g.vertices.take(1)(0)
    val visisted = Bfs.run(g, startVertexId)
  }
}
