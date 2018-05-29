import org.apache.spark.SparkContext
import org.apache.spark.graphx.{Graph, VertexId}

object Bfs {
  def run[ED](graph: Graph[Boolean, ED], start: VertexId)(implicit sc: SparkContext): Unit = {
    graph
      .pregel(false)((vid, vdata, msg) => {
        // initial iteration
        if (!msg) vid == start
        // any other iteration, if we received any message it means we're visited
        else true
      }, triplet => {
        if (triplet.srcAttr) Iterator.single((triplet.dstId, true))
        else Iterator.empty
      }, (m1, m2) =>
        // only messages in circulation are 'visited' messages, we could OR them, but this is simple
        true
      )
  }
}
