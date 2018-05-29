import org.apache.spark.{SparkConf, SparkContext}

object LocalRunner {
  def main(args: Array[String]) = {
    val sparkConf = new SparkConf().setAppName("example-graphx").setMaster("local[2]")
    implicit val sc = new SparkContext(sparkConf)

    Main.run(sc)

    sc.stop()
  }
}
