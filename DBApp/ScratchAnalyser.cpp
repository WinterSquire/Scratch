#include <QDir>
#include <bitset>
#include <fstream>

#include <Scratch.hpp>
#include "ScratchAnalyser.hpp"

#include "DatabaseManager.hpp"

extern "C"
{
	extern const char index_html_start[];
	extern const char index_html_end[];
}


union Rectangle
{
	struct { int x1, y1, x2, y2; };
	struct { int x, y, width, height; };
};

#define MAX_AXIS_SIZE 224

inline static Rectangle getDim(const std::vector<QPoint>& fields)
{	
	Rectangle result{};
	std::bitset<MAX_AXIS_SIZE> rows, columns;

	for (auto field : fields)
	{
		rows[field.y()] = true;
		columns[field.x()] = true;
	}

	for (int i = 0; i < MAX_AXIS_SIZE; ++i)
	{
		if (rows[i])
		{
			if (!result.y1)
				result.y1 = i;
			++result.height;
		}

		if (columns[i])
		{
			if (!result.x1)
				result.x1 = i;
			++result.width;
		}
	}

	ASSERT(fields.size() == result.width * result.height);

	return result;
}

static QString ScratchAnalyser::process(
	const struct ExperimentInfo& experiment, 
	const struct ScratchParameterGlobal& parameter,
	ScratchAnalyser::QuerySamplesPFN queryFunction)
{
	cv::Mat images[3];
	QList<SampleInfo> samples;
	ScratchParameterKineticOnce _parameter;
	ScratchParameterKinetic __parameter;
	auto numberOfSequence = experiment.cycleCount;
	std::vector<ScratchResult> frames(numberOfSequence);
	std::vector<uint64_t> timestamps(numberOfSequence);
	
	_parameter.image = images + 0;
	_parameter.debugImages[0] = images + 1;
	_parameter.debugImages[1] = images + 2;
	
	auto workDir = QDir::tempPath();

	// process each group
	for (auto& group : experiment.scanAreaGroups)
	{
		auto dimOfSample = getDim(group.vFields);
		auto numberOfSample = group.vFields.size();
		
		QList<cv::Mat> sampleImages(numberOfSample);
		QList<cv::Mat> sampleRowImages(dimOfSample.height);

		// process each well
		for (auto& wellName : group.vstrWells)
		{
			auto wellDir = QString("%1/%2/%3").arg(group.strGroupName).arg(workDir).arg(wellName).toStdString();

			for (int sequenceIndex = 0; sequenceIndex < numberOfSequence; ++sequenceIndex)
			{
				uint64_t wellTime = 0;
				
				// querySamples
				auto samples = queryFunction(experiment.experimentId, wellName, sequenceIndex);

				ASSERT(samples.size() == numberOfSample);

				// read sample images
				for (auto& sample : samples)
				{
					auto sampleIndex = (sample.fieldY - dimOfSample.y) * dimOfSample.width + (sample.fieldX - dimOfSample.x);
					sampleImages[sampleIndex] = cv::imread(sample.segmentationResultPath.toStdString());
					timestamps[sequenceIndex] += sample.captureTime.toSecsSinceEpoch();
				}

				timestamps[sequenceIndex] /= samples.size();

				// config parameter
				if (!sequenceIndex)
				{
					_parameter.p = 1.0;
					_parameter.t50 = _parameter.t90 = 0;
					_parameter.frames[FrameCurrent] = _parameter.frames[FramePrevious] = _parameter.frames[FrameFirst] = frames.data() + 0;
					_parameter.timestamps[FrameCurrent] = _parameter.timestamps[FramePrevious] = _parameter.timestamps[FrameFirst] = timestamps[sequenceIndex];
				}
				else
				{
					_parameter.frames[FramePrevious] = _parameter.frames[FrameCurrent]++;
					_parameter.timestamps[FramePrevious] = _parameter.timestamps[FrameCurrent];
					_parameter.timestamps[FrameCurrent] = timestamps[sequenceIndex];
				}
				
				// concat images
				for (int i = 0; i < dimOfSample.height; ++i)
					cv::hconcat(sampleImages.data() + (i * dimOfSample.width), dimOfSample.width, sampleRowImages.data()[i]);
				cv::vconcat(sampleRowImages.data(), dimOfSample.height, images[0]);

				// analyse image
				int analyseErrorCode = CScratchController::analyseScratchKineticOnce(_parameter, parameter);
				ASSERT(analyseErrorCode == ScratchErrorSuccess);

				// write images
				auto wellImageDir = QString("%1/images/%2").arg(wellDir).arg(QString::number(wellTime)).toStdString();
				cv::imwrite(wellImageDir + "/0.png", images[0]);
				cv::imwrite(wellImageDir + "/1.png", images[1]);
				cv::imwrite(wellImageDir + "/2.png", images[2]);
			}

			// write data
			std::ofstream data_json(wellDir + "/data.json");
			std::ofstream index_html(wellDir + "/index.html");
			index_html.write(index_html_start, (size_t)(index_html_end - index_html_start));

			__parameter.p = _parameter.p;
			__parameter.t50 = _parameter.t50;
			__parameter.t90 = _parameter.t90;
			__parameter.frames = frames.data();
			__parameter.timestamps = timestamps.data();

			CDataJsonSerializer().serialize(
				numberOfSequence,
				&__parameter,
				NULL,
				parameter,
				data_json
			);
		}
	}

	return 0;
}
