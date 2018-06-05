package perftest

import org.apache.log4j.{Level, Logger}
import org.apache.spark._
import org.apache.spark.graphx._
import org.apache.spark.storage.StorageLevel

import scala.io.Source

object Main {
  object Algorithm extends Enumeration {
    type Algorithm = Value
    val Bfs, Colouring = Value
  }

  // Start Spark.
  def run(algo: Algorithm.Value)(implicit sc: SparkContext) = {
    // Suppress unnecessary logging.
    Logger.getRootLogger.setLevel(Level.ERROR)

    val g: Graph[Int, Int] = Utils.loadGraphFromStdDirectory("powerlaw_25_2_05_876")

    // Calculate centralities.

    println(s"\n### $algo\n")

    val duration = algo match {
      case Algorithm.Bfs =>
        val bg = g.mapVertices((vid, _) => false)
        val (startVertexId, _) = g.vertices.take(1)(0)

        val start = System.nanoTime()
        Bfs.run(bg, startVertexId)
        System.nanoTime() - start

      case Algorithm.Colouring =>
        val start = System.nanoTime()
        Colouring.run(g)
        System.nanoTime() - start
    }

    println(s"$duration ns")
  }
}
