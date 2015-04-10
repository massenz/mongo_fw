//
// Created by Marco Massenzio on 4/6/15.
//

#ifndef MONGO_FRAMEWORK_MONGOEXECUTOR_H
#define MONGO_FRAMEWORK_MONGOEXECUTOR_H

#include <mesos/executor.hpp>


class MongoExecutor : public mesos::Executor
{

public:
	MongoExecutor() {}
	virtual ~MongoExecutor();
};


#endif //MONGO_FRAMEWORK_MONGOEXECUTOR_H
