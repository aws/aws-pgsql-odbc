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

import org.testcontainers.containers.output.BaseConsumer;
import org.testcontainers.containers.output.OutputFrame;

public class ConsoleConsumer extends BaseConsumer<org.testcontainers.containers.output.Slf4jLogConsumer> {

  private final boolean separateOutputStreams;

  public ConsoleConsumer() {
    this(false);
  }

  public ConsoleConsumer(boolean separateOutputStreams) {
    this.separateOutputStreams = separateOutputStreams;
  }

  @Override
  public void accept(OutputFrame outputFrame) {
    final OutputFrame.OutputType outputType = outputFrame.getType();

    final String utf8String = outputFrame.getUtf8String();

    switch (outputType) {
      case END:
        break;
      case STDOUT:
        System.out.print(utf8String);
        break;
      case STDERR:
        if (separateOutputStreams) {
          System.err.print(utf8String);
        } else {
          System.out.print(utf8String);
        }
        break;
      default:
        throw new IllegalArgumentException("Unexpected outputType " + outputType);
    }
  }
}
