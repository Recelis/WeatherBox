import { CfnOutput, Stack, StackProps } from "aws-cdk-lib";
import { Construct } from "constructs";
// https://docs.aws.amazon.com/solutions/latest/constructs/aws-iot-lambda.html
import {
  IotToLambdaProps,
  IotToLambda,
} from "@aws-solutions-constructs/aws-iot-lambda";
import * as lambda from "aws-cdk-lib/aws-lambda";
import { ThingWithCert } from "cdk-iot-core-certificates";
import { things } from "../things/create-things";
import { PolicyStatement } from "aws-cdk-lib/aws-iam";
import {
  AwsCustomResource,
  AwsCustomResourcePolicy,
  PhysicalResourceId,
} from "aws-cdk-lib/custom-resources";

export const stackName = "ESP32Monitoring";

export class CdkStack extends Stack {
  constructor(scope: Construct, id: string, props?: StackProps) {
    super(scope, id, props);

    const getIoTEndpoint = new AwsCustomResource(this, "IoTEndpoint", {
      onCreate: {
        service: "Iot",
        action: "describeEndpoint",
        physicalResourceId: PhysicalResourceId.fromResponse("endpointAddress"),
        parameters: {
          endpointType: "iot:Data-ATS",
        },
      },
      policy: AwsCustomResourcePolicy.fromSdkCalls({
        resources: AwsCustomResourcePolicy.ANY_RESOURCE,
      }),
    });

    // Need to do a custom resource lookup
    const AWS_IOT_ENDPOINT = getIoTEndpoint.getResponseField("endpointAddress");

    // Use a L3 Construct to create thing and generate certs associated with Thing
    // should saved in AWS parameter store. This is provided by AWS, can't really do it with
    // normal L2 constructs from what I've found.

    // Creates new AWS IoT Thing called thingName
    // Saves certs to /devices/thingName/certPem and /devices/thingName/privKey
    // thingName and paramPrefix cannot start with '/'
    // { thingArn, certId, certPem, privKey }

    for (const thing of things) {
      const thingWIthCert = new ThingWithCert(
        this,
        `ThingWithCert${thing.suffix}`,
        {
          thingName: `${stackName}IotThing${thing.suffix}`,
          saveToParamStore: true,
          paramPrefix: "devices",
        }
      );

      new CfnOutput(this, `Output-ThingArn-${thing.suffix}`, {
        value: thingWIthCert.thingArn,
      });

      new CfnOutput(this, `Output-CertId-${thing.suffix}`, {
        value: thingWIthCert.certId,
      });

      new CfnOutput(this, `Output-CertPem-${thing.suffix}`, {
        value: thingWIthCert.certPem,
      });

      new CfnOutput(this, `Output-PrivKey-${thing.suffix}`, {
        value: thingWIthCert.privKey,
      });
    }

    const accountId = this.account;
    const region = this.region;

    const iotPublishPolicy = new PolicyStatement({
      actions: ["iot:Publish"],
      resources: [`arn:aws:iot:${region}:${accountId}:topic/esp32/sub`],
    });

    const iotRuleLambda = new lambda.Function(
      this,
      `${stackName}IotRuleLambda`,
      {
        code: lambda.Code.fromAsset(`lambda`),
        runtime: lambda.Runtime.NODEJS_20_X,
        handler: "index.handler",
        environment: {
          AWS_IOT_ENDPOINT,
        },
      }
    );

    iotRuleLambda.addToRolePolicy(iotPublishPolicy);
    const iotToLambdaProps: IotToLambdaProps = {
      existingLambdaObj: iotRuleLambda,
      iotTopicRuleProps: {
        topicRulePayload: {
          ruleDisabled: false,
          description:
            "Processing of ESP32 Monitoring from the AWS Connected Solution.",
          sql: "SELECT * FROM 'esp32/pub'",
          actions: [],
        },
      },
    };

    new IotToLambda(this, `${stackName}IotLambdaIntegration`, iotToLambdaProps);
  }
}
