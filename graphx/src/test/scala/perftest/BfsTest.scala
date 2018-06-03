package perftest

import org.apache.spark.{SparkConf, SparkContext}
import org.apache.spark.graphx.Graph
import org.scalatest.FunSpec

class BfsTest extends FunSpec {
    describe("Bfs") {
        describe("on powerlaw graph") {
            it("should visit all vertices") {
                TestUtils.withTestContext(implicit sc => {
                  val g = Utils.loadGraphFromStdDirectory("powerlaw_25_2_05_876").mapVertices((vid, _) => false)
                  val (startVertexId, _) = g.vertices.take(1)(0)
                  val vertexData = Bfs.run(g, startVertexId).vertices.map({ case (vid, visited) => visited })

                  assert(vertexData.count() === g.numVertices, "(all vertices must be visisted)")
                  assert(vertexData.reduce((a,b) => a && b), "(all recorded visits must be unique)")
                })
            }
        }
    }
}