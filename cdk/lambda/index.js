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
    payload: Buffer.from(JSON.stringify(event)), // e.g. Buffer.from("") or new TextEncoder().encode("")
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

/*
const iotdata = new IotData({
  endpoint: process.env.AWS_IOT_ENDPOINT,
});

exports.handler = async (event) => {
  console.log("Event => " + JSON.stringify(event));
  const params = {
    topic: "esp32/sub",
    payload: JSON.stringify(event),
    qos: 0,
  };
  console.log(process.env.AWS_IOT_ENDPOINT);
  return iotdata
    .publish(params, function (err, data) {
      if (err) {
        console.log("ERROR => " + JSON.stringify(err));
      } else {
        console.log("Success");
      }
    })
    .promise();
};
*/
