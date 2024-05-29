const {
  IoTDataPlaneClient,
  PublishCommand,
} = require("@aws-sdk/client-iot-data-plane");

exports.handler = async (event) => {
  const client = new IoTDataPlaneClient();
  event["timestamp"] = (new Date()).getTime();
  const input = {
    // PublishRequest
    topic: "esp32/sub", // required
    qos: 0,
    payload: Buffer.from(JSON.stringify(event))
  };
  const command = new PublishCommand(input);
  try {
    console.log("Sending data");
    const response = await client.send(command);
    console.log(response);
  } catch (error) {
    console.log(error);
  }
};
