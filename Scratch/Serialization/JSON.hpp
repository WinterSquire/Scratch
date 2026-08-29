#pragma once

#include "Serialization.hpp"

#include <QJsonDocument>
#include <QJsonObject>

#define toJsonString(data) (QJsonDocument(toJson(data)).toJson())
#define fromJsonString(data, json) fromJson(data, (QJsonDocument::fromJson(jsonUtf8)));

class CDataJsSerializer : public ISerializer
{
public:
    virtual int serialize(
        size_t size,
        struct ScratchParameterKinetic* exp,
        struct ScratchParameterKinetic* con,
        struct ScratchParameterGlobal& parameter, 
        std::ostream& os) override;
};

class CImagesJsSerializer : public ISerializer
{
public:
    virtual int serialize(
        size_t size,
        struct ScratchParameterKinetic* exp,
        struct ScratchParameterKinetic* con,
        struct ScratchParameterGlobal& parameter, 
        std::ostream& os) override;

    static QByteArray cvMatToBase64(const cv::Mat& mat);
};

QJsonObject toJson(const struct ScratchParameterGlobal& data);
int fromJson(struct ScratchParameterGlobal& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchParameterKinetic& data, size_t size);
int fromJson(struct ScratchParameterKinetic& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchResult& data);
int fromJson(struct ScratchResult& data, const QJsonObject& json);
