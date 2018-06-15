
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <random>

typedef unsigned int TVertexId;

class BackEdgeMap {
public:
	void add_back_edge(TVertexId from, TVertexId to) { map[from].insert(to); }
	std::unordered_set<TVertexId>& get_back_edges(TVertexId v_id) { return map[v_id]; }
	void forget_vertex(TVertexId v_id) { map.erase(v_id); }

	std::unordered_map<TVertexId, std::unordered_set<TVertexId>> map;
};


void g(TVertexId v_count, size_t edges_per_vertex_soft, std::ofstream& f_adjl, std::ofstream& f_elt, std::mt19937::result_type seed)
{
	std::mt19937 gen{std::random_device{}()};

	BackEdgeMap be_map;

	size_t edge_count = 0;
	TVertexId mask = 0xf;
	for(TVertexId src_v_id = 0; src_v_id < v_count; src_v_id++) {
		/* graphs for framework are requred to: for each (u, v) in G: (v, u) in G */
		/* to avoid duplicates, we only add edges to higher vertex ids */
		/* we also have to remember back edges - this is handled by BackEdgeMap */

		f_adjl << src_v_id << ' ';

		auto& back_edges = be_map.get_back_edges(src_v_id);

		/* generate some additional edges if needed */
		if (back_edges.size() < edges_per_vertex_soft) {
			auto to_generate = edges_per_vertex_soft - back_edges.size();
			to_generate *= 3;
			to_generate /= 2;

			std::uniform_int_distribution<TVertexId> distribution(src_v_id+1, v_count-1);
			for(size_t i = 0; i < to_generate; i++) {
				// generate neighbour
				TVertexId dest_v_id = distribution(gen);
				back_edges.insert(dest_v_id);

				// cache back edge
				be_map.add_back_edge(dest_v_id, src_v_id);
			}
		}

		/* write all neighbours */
		for(auto dest: back_edges) {
			f_adjl << dest << ' ';
			f_elt << src_v_id << '\t' << dest << '\n';
		}
		edge_count += back_edges.size();
		be_map.forget_vertex(src_v_id);

		f_adjl << '\n';

		if ((src_v_id & mask) == 0)
			std::cout << src_v_id << "/" << v_count << '\n';
	}

	std::cout << "edge_count = " << edge_count << '\n';
}

int main(int argc, const char **argv) {
	std::ofstream adjl("tgraph.adjl");
	std::ofstream elt("tgraph.elt");
	g(1000000, 10, adjl, elt, 6537);
}