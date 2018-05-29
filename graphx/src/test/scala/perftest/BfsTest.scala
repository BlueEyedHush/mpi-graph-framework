package perftest

import org.apache.spark.{SparkConf, SparkContext}
import org.apache.spark.graphx.Graph
import org.scalatest.FunSpec

class BfsTest extends FunSpec {
    describe("Bfs") {
        describe("on powerlaw graph") {
            it("should visit all vertices") {
                implicit val sc = new SparkContext(new SparkConf().setAppName("example-graphx").setMaster("local[2]"))

                try {
                    val g = Utils.loadGraphFromStdDirectory("powerlaw_25_2_05_876").mapVertices((vid, _) => false)
                    val (startVertexId, _) = g.vertices.take(1)(0)
                    val visisted = Bfs.run(g, startVertexId)

                    assert(visisted.size === g.numVertices, "(all vertices must be visisted)")
                    require(visisted.toSet.size === visisted.size, "(all recorded visits must be unique)")

                } finally sc.stop()
            }
        }
    }
}