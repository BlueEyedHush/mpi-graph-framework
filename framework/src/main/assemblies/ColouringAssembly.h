//
// Created by blueeyedhush on 14.11.17.
//

#ifndef FRAMEWORK_COLOURINGASSEMBLY_H
#define FRAMEWORK_COLOURINGASSEMBLY_H

#include <Assembly.h>
#include <validators/ColouringValidator.h>


template <template <typename> class TColouring, typename TGHandle>
class ColouringAssembly : public AlgorithmAssembly<TGHandle, TColouring, ColouringValidator> {
	using G = typename TGHandle::GPType;

public:
	ColouringAssembly(TGHandle& graphHandle) : h(graphHandle), algo(nullptr), validator(nullptr) {}

	~ColouringAssembly() {
		if (algo != nullptr) {delete algo;}
		if (validator != nullptr) {delete validator;}
	}

protected:
	virtual TGHandle& getHandle() override {
		return h;
	};

	virtual TColouring<G>& getAlgorithm(TGHandle&) override {
		algo = new TColouring<G>();
		return *algo;
	};

	virtual ColouringValidator<G>& getValidator(TGHandle&, TColouring<G>&) override {
		validator = new ColouringValidator<G>();
		return *validator;
	};

private:
	TGHandle& h;
	TColouring<G> *algo;
	ColouringValidator<G> *validator;
};

#endif //FRAMEWORK_COLOURINGASSEMBLY_H
