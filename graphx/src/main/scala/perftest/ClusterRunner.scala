package perftest

import org.apache.log4j.{Level, Logger}
import org.apache.spark.{SparkConf, SparkContext}

object ClusterRunner {
  def stdG(name: String) = s"../graphs/data/$name.elt"

  def main(args: Array[String]): Unit = {
    Logger.getRootLogger.setLevel(Level.ERROR)

    val sparkConf = new SparkConf().setAppName("example-graphx")
    implicit val sc = new SparkContext(sparkConf)

    val cliArgs = CliParser.parseCli(args.toList)

    val graphs = List(
      stdG("powergraph_100000_999719"),
      stdG("powergraph_200000_1999678"),
      stdG("powergraph_300000_2999660"),
      stdG("powergraph_400000_3999643"),
      stdG("powergraph_500000_4999634")
    )

    graphs.foreach(g => Main.run(cliArgs.algorithm, 1, g))

    sc.stop()
  }
}
