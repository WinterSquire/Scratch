#pragma once

#include <QList>
#include <QString>

struct SampleInfo;
struct ExperimentInfo;
struct ScratchParameterGlobal;

namespace ScratchAnalyser
{
    typedef QList<struct SampleInfo> (*QuerySamplesPFN)(const QString& experimentId, const QString& wellName, int cycleIndex);

    static QString process(
        const struct ExperimentInfo& experiment, 
	    const struct ScratchParameterGlobal& parameter,
        QuerySamplesPFN queryFunction);
}