// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef HOSTAVAILABILITYSTRATEGY_H_
#define HOSTAVAILABILITYSTRATEGY_H_

typedef enum {
  AVAILABLE,
  NOT_AVAILABLE
} HostAvailability;

class HostAvailabilityStrategy {
public:
    virtual ~HostAvailabilityStrategy() = default;

    virtual void SetHostAvailability(HostAvailability hostAvailability) = 0;
    virtual HostAvailability GetHostAvailability(HostAvailability rawHostAvailability) = 0;
};

#endif // HOSTAVAILABILITYSTRATEGY_H_
