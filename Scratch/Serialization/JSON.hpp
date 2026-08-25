#pragma once

#include <QJsonDocument>
#include <QJsonObject>

#define toJsonString(data) (QJsonDocument(toJson(data)).toJson(QJsonDocument::Compact))
#define fromJsonString(data, json) fromJson(data, (QJsonDocument::fromJson(jsonUtf8)));

QJsonObject toJson(const struct ScratchParameter& data);
int fromJson(struct ScratchParameter& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchArea& data);
int fromJson(struct ScratchArea& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchInvasionData& data);
int fromJson(struct ScratchInvasionData& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchResult& data);
int fromJson(struct ScratchResult& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchResultFrame& data);
int fromJson(struct ScratchResultFrame& data, const QJsonObject& json);
