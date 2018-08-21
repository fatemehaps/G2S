/*
 * G2S (c) by Mathieu Gravey (gravey.mathieu@gmail.com)
 * 
 * G2S is licensed under a
 * Creative Commons Attribution-NonCommercial 4.0 International License.
 * 
 * You should have received a copy of the license along with this
 * work. If not, see <http://creativecommons.org/licenses/by-nc/4.0/>.
 */

#ifndef SAMPLING_MODULE_HPP
#define SAMPLING_MODULE_HPP

#include <functional>
#include "typeDefine.hpp"

class SamplingModule {
public:
	struct matchLocation{
		unsigned TI;
		unsigned index;
	};

	struct narrownessMeasurment{
		matchLocation candidate;
		float narrowness;
	};


protected:
	std::vector<ComputeDeviceModule*> *_cdmV;
	g2s::DataImage* _kernel;
	std::function<float(float*, unsigned int *, unsigned int * , unsigned int )> _narrownessFunction;
public:
	SamplingModule(std::vector<ComputeDeviceModule *> *cdmV, g2s::DataImage *kernel){
		_cdmV=cdmV;
		_kernel=kernel;
	};
	~SamplingModule(){};

	void setNarrownessFunction(std::function<float(float*, unsigned int *, unsigned int * , unsigned int )> narrownessFunction){
		_narrownessFunction=narrownessFunction;
	}

	virtual matchLocation sample(std::vector<std::vector<int>> neighborArrayVector, std::vector<std::vector<float> > neighborValueArrayVector,float seed, matchLocation verbatimRecord, unsigned moduleID=0, bool fullStationary=false)=0;
	virtual narrownessMeasurment narrowness(std::vector<std::vector<int>> neighborArrayVector, std::vector<std::vector<float> > neighborValueArrayVector,float seed, unsigned moduleID=0, bool fullStationary=false)=0;
};

#endif // SAMPLING_MODULE_HPP