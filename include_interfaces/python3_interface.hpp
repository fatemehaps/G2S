/*
 * G2S
 * Copyright (C) 2018, Mathieu Gravey (gravey.mathieu@gmail.com) and UNIL (University of Lausanne)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PYTHON_3_INTERFACE_HPP
#define PYTHON_3_INTERFACE_HPP

#include <Python.h>
#include <numpy/arrayobject.h>
#include "inerfaceTemplate.hpp"

class InerfaceTemplatePython3: public InerfaceTemplate
{
private:
	PyThreadState *_save;
public:

	void unlockThread(){
		_save = PyEval_SaveThread();
	}
	void lockThread(){
		PyEval_RestoreThread(_save);
		_save=nullptr;
	}

	bool userRequestInteruption(){
		lockThread();
		bool status=PyErr_CheckSignals();
		unlockThread();
		return status;
	}

	bool isDataMatrix(std::any val){
		return PyArray_Check(std::any_cast<PyObject *>(val));
	}

	std::string nativeToStandardString(std::any val){
		PyObject * pyObj=std::any_cast<PyObject *>(val);
		if(PyUnicode_Check(pyObj))
			return std::string(PyUnicode_AsUTF8(pyObj)); //mxGetString
		if(PyFloat_Check(pyObj))
			return std::to_string(float(PyFloat_AsDouble(pyObj)));
		if(PyLong_Check(pyObj))
			return std::to_string(unsigned(PyLong_AsLong(pyObj)));
		return "";
	}

	double nativeToScalar(std::any val){
		return PyFloat_AsDouble(std::any_cast<PyObject *>(val));
	}

	unsigned nativeToUint32(std::any val){
		return PyLong_AsLong(std::any_cast<PyObject *>(val));
	};

	std::any ScalarToNative(double val){
		return std::any(PyFloat_FromDouble(val));
	}

	std::any Uint32ToNative(unsigned val){
		return std::any(PyLong_FromUnsignedLong(val));
	};

	void sendError(std::string val){
		lockThread();
		PyErr_Format(PyExc_KeyboardInterrupt,"%s ==> %s","g2s:error", val.c_str()); //PyExc_Exception
		unlockThread();
	}

	void sendWarning(std::string val){
		lockThread();
		PyErr_WarnFormat(PyExc_Warning,2,"%s ==> %s","g2s:warning", val.c_str());
		unlockThread();
	}

	void eraseAndPrint(std::string val){
		printf("\r%s        ",val.c_str());
	}

	std::any convert2NativeMatrix(g2s::DataImage &image){
		npy_intp* dimsArray=new npy_intp[image._dims.size()+1];
		for (size_t i = 0; i < image._dims.size(); ++i)
		{
			dimsArray[i]=image._dims[i];
		}
		std::reverse(dimsArray,dimsArray+image._dims.size());
		dimsArray[image._dims.size()]=image._nbVariable;
		
		PyObject *array=nullptr;
		if(image._encodingType==g2s::DataImage::Float)
			array=PyArray_SimpleNew(image._dims.size()+(image._nbVariable>1), dimsArray,NPY_FLOAT);
		if(image._encodingType==g2s::DataImage::Integer)
			array=PyArray_SimpleNew(image._dims.size()+(image._nbVariable>1), dimsArray,NPY_INT32);
		if(image._encodingType==g2s::DataImage::UInteger)
			array=PyArray_SimpleNew(image._dims.size()+(image._nbVariable>1), dimsArray,NPY_UINT32);
		delete[] dimsArray;
		float* data=(float*)PyArray_DATA(array);
		//unsigned nbOfVariable=image._nbVariable;
		unsigned dataSize=image.dataSize();
		/*for (int i = 0; i < dataSize/nbOfVariable; ++i)
		{
			for (int j = 0; j < nbOfVariable; ++j)
			{
				data[i+j*(dataSize/nbOfVariable)]=image._data[i*nbOfVariable+j];
			}
		}*/

		memcpy(data,image._data,dataSize*sizeof(float));

		return array;
	}


	g2s::DataImage convertNativeMatrix2DataImage(std::any matrix, std::any dataType=nullptr){
		PyObject * prh=std::any_cast<PyObject *>(matrix);
		PyObject * variableTypeArray=nullptr;
		if(dataType.type()==typeid(PyObject *))
			variableTypeArray=std::any_cast<PyObject *>(dataType);
		int dataSize=PyArray_SIZE(prh);
		int nbOfVariable=1;
		if(variableTypeArray)nbOfVariable=PyArray_SIZE(variableTypeArray);
		int dimData = PyArray_NDIM(prh)-(nbOfVariable>1);
		const npy_intp * dim_array = PyArray_DIMS(prh);
		unsigned *dimArray=new unsigned[dimData];
		for (int i = 0; i < dimData; ++i)
		{
			dimArray[i]=dim_array[i];
		}

		std::reverse(dimArray,dimArray+dimData);


		g2s::DataImage image(dimData,dimArray,nbOfVariable);
		delete[] dimArray;
		float *data=image._data;
		
		
		if (variableTypeArray && PyArray_TYPE(variableTypeArray)==NPY_FLOAT)
		{
			float* ptrVarType=(float *)PyArray_DATA(variableTypeArray);
			for (int i = 0; i < nbOfVariable; ++i)
			{
				if(ptrVarType[i]==0.f)image._types[i]=g2s::DataImage::VaraibleType::Continuous;
				if(ptrVarType[i]==1.f)image._types[i]=g2s::DataImage::VaraibleType::Categorical;
			}
		}

		if (variableTypeArray && PyArray_TYPE(variableTypeArray)==NPY_DOUBLE)
		{
			double* ptrVarType=(double *)PyArray_DATA(variableTypeArray);
			for (int i = 0; i < nbOfVariable; ++i)
			{
				if(ptrVarType[i]==0.)image._types[i]=g2s::DataImage::VaraibleType::Continuous;
				if(ptrVarType[i]==1.)image._types[i]=g2s::DataImage::VaraibleType::Categorical;
			}
		}
		
		memset(data,0,sizeof(float)*dataSize);
		//manage data
		if(PyArray_TYPE(prh)==NPY_DOUBLE){
			double *matrixData=(double *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		if(PyArray_TYPE(prh)==NPY_FLOAT){
			float *matrixData=(float *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		if(PyArray_TYPE(prh)==NPY_UINT8){
			uint8_t *matrixData=(uint8_t *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		if(PyArray_TYPE(prh)==NPY_UINT16){
			uint16_t *matrixData=(uint16_t *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		if(PyArray_TYPE(prh)==NPY_UINT32){
			uint32_t *matrixData=(uint32_t *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		if(PyArray_TYPE(prh)==NPY_UINT64){
			uint64_t *matrixData=(uint64_t *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		if(PyArray_TYPE(prh)==NPY_INT8){
			int8_t *matrixData=(int8_t *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		if(PyArray_TYPE(prh)==NPY_INT16){
			int16_t *matrixData=(int16_t *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		if(PyArray_TYPE(prh)==NPY_INT32){
			int32_t *matrixData=(int32_t *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		if(PyArray_TYPE(prh)==NPY_INT64){
			int64_t *matrixData=(int64_t *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}

		if(PyArray_TYPE(prh)==NPY_BOOL){
			bool *matrixData=(bool *)PyArray_DATA(prh);
			for (int i = 0; i < dataSize; ++i)
			{
				data[i]=matrixData[i];
			}
		}
		return image;
	}

	void addInputsToInputsMap(std::multimap<std::string, std::any> &inputs, std::string key, PyObject* value){
		if(PyTuple_Check(value)){
			for (int p = 0; p < PyTuple_Size(value) ; ++p)
			{
				inputs.insert(std::pair<std::string, std::any>(key,std::any(PyTuple_GetItem(value,p))));
			}
			return;
		}
		if(PyList_Check(value)){
			for (int p = 0; p < PyList_Size(value) ; ++p)
			{
				inputs.insert(std::pair<std::string, std::any>(key,std::any(PyList_GetItem(value,p))));
			}
			return;
		}
		if(PySet_Check(value)){
			auto it = PyObject_GetIter(value);
		    if (it == NULL)
		        return;
		    PyObject* obj;
		    while ((obj = PyIter_Next(it)) != NULL) {
				inputs.insert(std::pair<std::string, std::any>(key,std::any(obj)));
		        Py_DECREF(obj);
		    }
		    Py_DECREF(it);
			return;
		}
		if(PyArray_Check(value) && PyArray_ISOBJECT(value)){
			for (int p = 0; p < PyArray_SIZE(value) ; ++p)
			{
				inputs.insert(std::pair<std::string, std::any>(key,std::any(PyArray_GETITEM(value,PyArray_GETPTR1(value,p)))));
			}
			return;
		}
		
		inputs.insert(std::pair<std::string, std::any>(key,std::any(value)));
	}


	PyObject * runStandardCommunicationPython(PyObject *self, PyObject *args, PyObject *keywds, int numberOfOutput=INT_MAX){
		setbuf(stdout, NULL);
		std::multimap<std::string, std::any> inputs;
		std::multimap<std::string, std::any> outputs;

		// the tuple
		if(args && PyTuple_Check(args)){
			std::vector<int> listOfIndex;
			for (int i = 0; i < PyTuple_Size(args); ++i)
			{
				if(PyUnicode_Check(PyTuple_GetItem(args,i))){
					std::string str=std::string(PyUnicode_AsUTF8(PyTuple_GetItem(args,i)));
					if( str.size()>1 && str.at(0)=='-' ) listOfIndex.push_back(i);
				}
			}
			listOfIndex.push_back(PyTuple_Size(args));

			for (size_t j = 0; j < listOfIndex.size()-1; ++j)
			{
				if(listOfIndex[j]+1==listOfIndex[j+1]){
					inputs.insert(std::pair<std::string, std::any>(std::string(PyUnicode_AsUTF8(PyTuple_GetItem(args,listOfIndex[j]))),nullptr));
				}else{
					//("%s\n",std::string(PyUnicode_AsUTF8(PyTuple_GetItem(args,listOfIndex[j]))).c_str());
					for (int i = listOfIndex[j]+1; i < listOfIndex[j+1]; ++i)
					{
						addInputsToInputsMap(inputs,std::string(PyUnicode_AsUTF8(PyTuple_GetItem(args,listOfIndex[j]))),PyTuple_GetItem(args,i));
					}
				}
			}
		}

		// keywords

		if(keywds && PyDict_CheckExact(keywds)){
			PyArg_ValidateKeywordArguments(keywds);
			PyObject *key, *value;
			Py_ssize_t pos = 0;

			while (PyDict_Next(keywds, &pos, &key, &value)) {
				addInputsToInputsMap(inputs,std::string("-")+std::string(PyUnicode_AsUTF8(key)),value);
			}
		}


		runStandardCommunication(inputs, outputs, numberOfOutput);

		if(outputs.size()==0){
			return Py_None;
		}

		int nlhs=std::min(numberOfOutput,std::max(int(outputs.size())-1,1));
		//printf("requested output %d\n",nlhs);
		PyObject* pyResult=PyTuple_New(nlhs);
		int position=0;
		for (int i=0; i < nlhs; ++i)
		{
			auto iter=outputs.find(std::to_string(i+1));
			if(iter!=outputs.end() && position<std::max(nlhs-1,1))
			{
				PyTuple_SetItem(pyResult,position,std::any_cast<PyObject*>(iter->second));
				Py_INCREF(PyTuple_GetItem(pyResult,position));
				position++;
			}else break;
		}

		if(position<nlhs){
			auto iter=outputs.find("t");
			if(iter!=outputs.end())
			{
				PyTuple_SetItem(pyResult,position,std::any_cast<PyObject*>(iter->second));
				Py_INCREF(PyTuple_GetItem(pyResult,position));
				position++;
			}
		}

		if(position<nlhs){
			auto iter=outputs.find("progression");
			if(iter!=outputs.end())
			{
				PyTuple_SetItem(pyResult,position,std::any_cast<PyObject*>(ScalarToNative(std::any_cast<float>(iter->second))));
				position++;
			}
		}


		if(position<nlhs){
			auto iter=outputs.find("id");
			if(iter!=outputs.end())
			{
				PyTuple_SetItem(pyResult,position,std::any_cast<PyObject*>(iter->second));
				Py_INCREF(PyTuple_GetItem(pyResult,position));
				position++;
			}
		}

		for (auto it=outputs.begin(); it!=outputs.end(); ++it){
    		if(it->second.type()==typeid(PyObject *)){
    			Py_DECREF(std::any_cast<PyObject*>(it->second));
    		}
		}


		if(nlhs==1){
			PyObject* pyResultUnique=PyTuple_GetItem(pyResult,0);
			Py_INCREF(pyResultUnique);
			Py_DECREF(pyResult);
			return pyResultUnique;
		}
		if(nlhs==0){
			Py_DECREF(pyResult);
			return Py_None;
		}


		return pyResult;
	}
};


#endif // PYTHON_3_INTERFACE_HPP