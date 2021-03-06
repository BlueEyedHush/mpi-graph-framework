//
// Created by blueeyedhush on 14.11.17.
//

#ifndef FRAMEWORK_BFSASSEMBLY_H
#define FRAMEWORK_BFSASSEMBLY_H

#include <Assembly.h>
#include <validators/BfsValidator.h>


template <template <typename> class TBfs, typename TGHandle>
class BfsAssembly : public AlgorithmAssembly<TGHandle, TBfs, BfsValidator> {
	using G = typename TGHandle::GPType;

public:
	BfsAssembly(TGHandle& graphHandle) : h(graphHandle), bfs(nullptr), validator(nullptr) {}

	~BfsAssembly() {
		if (bfs != nullptr) {delete bfs;}
		if (validator != nullptr) {delete validator;}
	}

protected:
	virtual TGHandle& getHandle() override {
		return h;
	};

	virtual TBfs<G>& getAlgorithm(TGHandle&) override {
		auto bfsRoot = h.getConvertedVertices()[0];
		bfs = new TBfs<G>(bfsRoot);
		return *bfs;
	};

	virtual BfsValidator<G>& getValidator(TGHandle&, TBfs<G>&) override {
		auto bfsRoot = h.getConvertedVertices()[0];
		validator = new BfsValidator<G>(bfsRoot);
		return *validator;
	};

private:
	TGHandle& h;
	TBfs<G> *bfs;
	BfsValidator<G> *validator;
};

#endif //FRAMEWORK_BFSASSEMBLY_H
