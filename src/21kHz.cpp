#include "21kHz.hpp"

Plugin *pluginInstance;

void init(Plugin *p) {
	pluginInstance = p;

  p->addModel(modelPalmLoop);
	p->addModel(modelD_Inf);
  p->addModel(modelTachyonEntangler);

}
