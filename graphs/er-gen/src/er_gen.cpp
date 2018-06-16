
#include <cstdlib>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <random>

typedef uint32_t TVertexId;

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
	auto to_generate_multip = 1.5;

	/* write vertex count at the beginning of adjl */
	f_adjl << v_count << '\n';
	/* write placeholder for edge count, save file position to replace it later */
	auto max_e_count = static_cast<size_t>(v_count*edges_per_vertex_soft*(to_generate_multip + 0.1));
	auto edge_count_spos = f_adjl.tellp();
	f_adjl << max_e_count << '\n';
	auto edge_count_epos = f_adjl.tellp();

	BackEdgeMap be_map;

	size_t edge_count = 0;
	TVertexId mask = 0xff;
	for(TVertexId src_v_id = 0; src_v_id < v_count; src_v_id++) {
		/* graphs for framework are requred to: for each (u, v) in G: (v, u) in G */
		/* to avoid duplicates, we only add edges to higher vertex ids */
		/* we also have to remember back edges - this is handled by BackEdgeMap */

		f_adjl << src_v_id << ' ';

		auto& back_edges = be_map.get_back_edges(src_v_id);

		/* generate some additional edges if needed */
		if (back_edges.size() < edges_per_vertex_soft) {
			auto to_generate = static_cast<size_t>((edges_per_vertex_soft - back_edges.size())*to_generate_multip);

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

	/* write correct edge count */
	auto placeholder_len = edge_count_epos - edge_count_spos;
	std::string placeholder(std::to_string(edge_count));
	while (placeholder.size() < placeholder_len - 1)
		placeholder.push_back(' ');
	f_adjl.seekp(edge_count_spos);
	f_adjl << placeholder << '\n';

	std::cout << "edge_count = " << edge_count << '\n';
}

int main(int argc, const char **argv) {
	if (argc < 3) {
		printf("Usage: er_gen <vertex_count> <edges_per_vertex>\n");
		exit(1);
	}

	TVertexId vCount = std::stoull(argv[1]);
	size_t edgesPerVertex = std::stoull(argv[2]);
	std::string graphName("cst_" + std::to_string(vCount) + "_" + std::to_string(edgesPerVertex));

	std::ofstream adjl(graphName + ".adjl");
	std::ofstream elt(graphName + ".elt");
	g(vCount, edgesPerVertex, adjl, elt, 6537);
}