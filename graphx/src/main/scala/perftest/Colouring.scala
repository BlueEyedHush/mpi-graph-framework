package perftest

import org.apache.spark.SparkContext
import org.apache.spark.graphx.{Graph, VertexId}

import scala.collection.mutable.ListBuffer
import scala.reflect.ClassTag

/**
  * This implementation assumes, that we deal with undirected graph
  * Since in GraphX all edges are directed, each "undirected" edge must
  * be represented as two directed edges
  *
  * Doesn't support loops
  */
object Colouring {

  case class VertexData[VD](idAdvSent: Boolean = false,
                            iAmWaitingFor: Long = 0,
                            waitingForMe: Set[VertexId] = Set.empty,
                            coloursAlreadyUsedUp: Set[Int] = Set.empty,
                            colouringPhaseStartedSent: Boolean = false,
                            colour: Option[Int] = None,
                            colourPropagatedToNeighbours: Boolean = false,
                            originalData: VD)

  trait Message
  case class NeighbourIdAdvertisment(id: Set[VertexId]) extends Message
  case class NeighbourChoseColour(colours: List[Int]) extends Message
  case object ColourPropagated extends Message
  case object Dummy extends Message
  case object ColouringPhaseStarted extends Message

  def run[VD: ClassTag, ED: ClassTag](graph: Graph[VD, ED])(implicit sc: SparkContext): Graph[VertexData[VD], ED] = {
    graph
      .mapVertices((_, vdata) => VertexData(originalData = vdata))
      .pregel[Message](Dummy)((vid, vdata, msg) => {
      
      var nvdata = vdata
      
      msg match {
        case NeighbourIdAdvertisment(idsSet) =>
          val (higherIdCount, lowerIdSet) = idsSet
            .map(nVid => if (nVid > vid) (1, Set.empty) else (0, Set(nVid)))
            .reduce((a,b) => (a,b) match { case ((c1, s1), (c2, s2)) => (c1 + c2, s1 ++ s2) })

          nvdata.copy(
            iAmWaitingFor = nvdata.iAmWaitingFor + higherIdCount,
            waitingForMe = nvdata.waitingForMe ++ lowerIdSet,
            idAdvSent = true /* since we received IdAdv it must be 2nd superstep, so we know that we sent
            ours even without confirmation only guys who didn't receive message (are not connected to anything)
            will have idAdvSent false, but it doesn't really matter */
          )

        case NeighbourChoseColour(colourSet) =>
          val stillWaitingFor = nvdata.iAmWaitingFor - colourSet.size

          val newChosenColour =
            if (stillWaitingFor == 0) {
              var colour: Int = 0
              val usedUpColoursIt = nvdata.coloursAlreadyUsedUp.toSeq.sorted.iterator
              while (usedUpColoursIt.hasNext && colour == usedUpColoursIt.next())
                colour += 1

              Some(colour)
            } else
              None

          nvdata.copy(
            coloursAlreadyUsedUp = nvdata.coloursAlreadyUsedUp ++ colourSet,
            colour = newChosenColour,
            iAmWaitingFor = stillWaitingFor
          )

        case ColouringPhaseStarted =>
          if (nvdata.iAmWaitingFor == 0)
            /* we don't need to wait for anybody, so we pick smallest colour - 0 */
            nvdata.copy(colour = Some(0), colouringPhaseStartedSent = true)
          else
            nvdata.copy(colouringPhaseStartedSent = true) // todo: guys that don't choose colour won't stop sending ColouringPhaseStarted

        case ColourPropagated =>
          nvdata.copy(colourPropagatedToNeighbours = true)

        case Dummy => nvdata
      }

    }, triplet => {
      triplet.srcAttr match {
        case VertexData(false, _, _, _, _, _, _, _) =>
          Iterator.single((triplet.dstId, NeighbourIdAdvertisment(Set(triplet.srcId))))
        case VertexData(true, _, _, _, false, None, false, _) =>
          /* sent after all ids has been adversited to jog vertices which don't have to wait to choose colour */
          Iterator.single((triplet.dstId, ColouringPhaseStarted))
        case VertexData(true, _, waitingForMe, _, _, Some(colour), false, _) =>
          /* we need to send message to target vertex, but also to src vertex,
             so that src vertex can update colourPropagatedToNeighbours flag
             (vertex program is only executed if there is any message incoming)
          */
          val toSrc = (triplet.srcId, ColourPropagated)

          if (waitingForMe.contains(triplet.dstId)) {
            val toDest = (triplet.dstId, NeighbourChoseColour(List(colour)))
            Iterator(toDest, toSrc)
          } else
            Iterator.single(toSrc)

        case _ =>
          Iterator.empty
      }
    }, {
      case (Dummy, Dummy) => Dummy
      case (NeighbourIdAdvertisment(s1), NeighbourIdAdvertisment(s2)) => NeighbourIdAdvertisment(s1 ++ s2)
      case (NeighbourChoseColour(s1), NeighbourChoseColour(s2)) => NeighbourChoseColour(s1 ++ s2)
      case (ColourPropagated, ColourPropagated) => ColourPropagated
      case (ColouringPhaseStarted, ColouringPhaseStarted) => ColouringPhaseStarted
      case (m1, m2) => throw new RuntimeException(s"received unexpected combination of messages:\n M1: $m1\n M2: $m2")
    }).mapVertices((id, vdata) => vdata.colour match {
      /* this is to set colour of unconnected vertices, for which there is no triplet */
      case None => vdata.copy(colour = Some(0))
      case _ => vdata
    })
  }

}
