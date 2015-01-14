/* -*- c++ -*- */

#define ANALYSIS_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "analysis_swig_doc.i"

%{
//////#include "analysis/file_meta_sink.h"
#include "analysis/stream_max.h"
//////#include "analysis/file_meta_sink.h"
#include "analysis/stream_max.h"
//////#include "analysis/file_meta_sink.h"
#include "analysis/stream_max.h"
//////#include "analysis/file_meta_sink.h"
#include "analysis/stream_max.h"
%}


//////%include "analysis/file_meta_sink.h"
//////GR_SWIG_BLOCK_MAGIC2(analysis, file_meta_sink);
//////%include "analysis/file_meta_sink.h"
//////GR_SWIG_BLOCK_MAGIC2(analysis, file_meta_sink);
//////%include "analysis/file_meta_sink.h"
//////GR_SWIG_BLOCK_MAGIC2(analysis, file_meta_sink);

%include "analysis/stream_max.h"
GR_SWIG_BLOCK_MAGIC2(analysis, stream_max);
