package perftest

import org.apache.spark.{SparkConf, SparkContext}

object ClusterRunner {
  def main(args: Array[String]): Unit = {
    val sparkConf = new SparkConf().setAppName("example-graphx")
    implicit val sc = new SparkContext(sparkConf)

    Main.run(sc)

    sc.stop()
  }
}
