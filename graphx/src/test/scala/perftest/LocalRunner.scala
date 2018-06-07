package perftest

import org.apache.spark.{SparkConf, SparkContext}
import perftest.Main.Algorithm

object LocalRunner {
  def main(args: Array[String]) = {
    val sparkConf = new SparkConf().setAppName("example-graphx").setMaster("local[2]")
    implicit val sc = new SparkContext(sparkConf)

    Main.run(Algorithm.Colouring, 3, "../graphs/data/SimpleTestGraph.elt")

    sc.stop()
  }
}
