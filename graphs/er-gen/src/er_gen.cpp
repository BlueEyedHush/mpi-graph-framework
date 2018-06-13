
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <iostream>

typedef unsigned int TVertexId;

class BackEdgeMap {
public:
	void add_back_edge(TVertexId from, TVertexId to) { map[from].push_back(to); }
	std::vector<TVertexId>& get_back_edges(TVertexId v_id) { return map[v_id]; }
	void forget_vertex(TVertexId v_id) { map.erase(v_id); }

	std::unordered_map<TVertexId, std::vector<TVertexId>> map;
};

void write_neighbour(std::ofstream& f_adjl, std::ofstream& f_elt, TVertexId src, TVertexId dest) {
	f_adjl << src << ' ';
	f_elt << src << '\t' << dest << '\n';
}

void g(TVertexId v_count, double p, std::ofstream& f_adjl, std::ofstream& f_elt, unsigned int seed) {
	srand(seed);
	int threshold = static_cast<int>(p*RAND_MAX);

	BackEdgeMap be_map;

	TVertexId mask = 0xf;
	for(TVertexId src_v_id = 0; src_v_id < v_count; src_v_id++) {
		/* graphs for framework are requred to: for each (u, v) in G: (v, u) in G */
		/* to avoid duplicates, we only add edges to higher vertex ids */
		/* we also have to remember back edges - this is handled by BackEdgeMap */

		f_adjl << src_v_id << ' ';

		/* write cached */
		for(auto dest: be_map.get_back_edges(src_v_id))
			write_neighbour(f_adjl, f_elt, src_v_id, dest);
		be_map.forget_vertex(src_v_id);

		for(TVertexId dest_v_id = src_v_id + 1; dest_v_id < v_count; dest_v_id++) {
			if (rand() <= threshold) {
				/* edge chosen */

				// write to files
				write_neighbour(f_adjl, f_elt, src_v_id, dest_v_id);

				// cache back edge
				be_map.add_back_edge(dest_v_id, src_v_id);
			}
		}

		f_adjl << '\n';

		if ((src_v_id & mask) == 0)
			std::cout << src_v_id << "/" << v_count << '\n';
	}
}

int main(int argc, const char **argv) {
	std::ofstream adjl("tgraph.adjl");
	std::ofstream elt("tgraph.elt");
	g(100000, 0.4, adjl, elt, 6537);
}