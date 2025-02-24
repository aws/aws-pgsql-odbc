// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

package utility;

import java.util.List;

public class AuroraClusterInfo {
  private final String clusterSuffix;
  private final String clusterEndpoint;
  private final String clusterROEndpoint;
  private final List<String> instances;

  public AuroraClusterInfo(
      String clusterSuffix,
      String clusterEndpoint,
      String clusterROEndpoint,
      List<String> instances) {
    this.clusterSuffix = clusterSuffix;
    this.clusterEndpoint = clusterEndpoint;
    this.clusterROEndpoint = clusterROEndpoint;
    this.instances = instances;
  }

  public String getClusterSuffix() {
    return clusterSuffix;
  }

  public String getClusterEndpoint() {
    return clusterEndpoint;
  }

  public String getClusterROEndpoint() {
    return clusterROEndpoint;
  }

  public List<String> getInstances() {
    return instances;
  }
}
