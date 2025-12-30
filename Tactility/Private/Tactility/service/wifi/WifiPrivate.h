#pragma once

#include <Tactility/service/ServiceContext.h>
#include <memory>

namespace tt::service::wifi {

std::shared_ptr<ServiceContext> findServiceContext();

}