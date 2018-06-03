package perftest

import org.apache.spark.{SparkConf, SparkContext}

object TestUtils {
  def withTestContext(code: SparkContext => Unit) = {
    implicit val sc = new SparkContext(new SparkConf()
      .setAppName("example-graphx")
      .setMaster("local[2]"))

    sc.setLogLevel("ERROR")

    try {
      code(sc)
    } finally sc.stop()
  }

}
