import { CfnOutput, Duration, Stack, StackProps } from "aws-cdk-lib";
import * as sns from "aws-cdk-lib/aws-sns";
import * as subs from "aws-cdk-lib/aws-sns-subscriptions";
import * as sqs from "aws-cdk-lib/aws-sqs";
import { Construct } from "constructs";
import {
  IotToLambdaProps,
  IotToLambda,
} from "@aws-solutions-constructs/aws-iot-lambda";
import * as lambda from "aws-cdk-lib/aws-lambda";
import { ThingWithCert } from "cdk-iot-core-certificates";

export const stackName = "ESP32Monitoring";

const constructProps: IotToLambdaProps = {
  lambdaFunctionProps: {
    code: lambda.Code.fromAsset(`lambda`),
    runtime: lambda.Runtime.NODEJS_20_X,
    handler: "index.handler",
  },
  // what is an iotTopicRule?
  iotTopicRuleProps: {
    topicRulePayload: {
      ruleDisabled: false,
      description:
        "Processing of ESP32 Monitoring from the AWS Connected Solution.",
      sql: "SELECT * FROM 'connectedcar/dtc/#'",
      actions: [],
    },
  },
};

export class CdkStack extends Stack {
  constructor(scope: Construct, id: string, props?: StackProps) {
    super(scope, id, props);

    // Use a L3 Construct to create thing and generate certs associated with Thing
    // should saved in AWS parameter store. This is provided by AWS, can't really do it with
    // normal L2 constructs from what I've found.

    // Creates new AWS IoT Thing called thingName
    // Saves certs to /devices/thingName/certPem and /devices/thingName/privKey
    // thingName and paramPrefix cannot start with '/'
    const { thingArn, certId, certPem, privKey } = new ThingWithCert(
      this,
      "ThingWithCert",
      {
        thingName: `${stackName}IotThing`,
        saveToParamStore: true,
        paramPrefix: "devices",
      }
    );

    new CfnOutput(this, "Output-ThingArn", {
      value: thingArn,
    });

    new CfnOutput(this, "Output-CertId", {
      value: certId,
    });

    new CfnOutput(this, "Output-CertPem", {
      value: certPem,
    });

    new CfnOutput(this, "Output-PrivKey", {
      value: privKey,
    });

    const queue = new sqs.Queue(this, `${stackName}Queue`, {
      visibilityTimeout: Duration.seconds(300),
    });

    const topic = new sns.Topic(this, `${stackName}Topic`);

    topic.addSubscription(new subs.SqsSubscription(queue));

    new IotToLambda(this, `${stackName}IotLambdaIntegration`, constructProps);
  }
}
