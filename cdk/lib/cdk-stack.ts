import { Duration, Stack, StackProps } from "aws-cdk-lib";
import * as sns from "aws-cdk-lib/aws-sns";
import * as subs from "aws-cdk-lib/aws-sns-subscriptions";
import * as sqs from "aws-cdk-lib/aws-sqs";
import { Construct } from "constructs";
import {
  IotToLambdaProps,
  IotToLambda,
} from "@aws-solutions-constructs/aws-iot-lambda";
import * as lambda from "aws-cdk-lib/aws-lambda";

const constructProps: IotToLambdaProps = {
  lambdaFunctionProps: {
    code: lambda.Code.fromAsset(`lambda`),
    runtime: lambda.Runtime.NODEJS_16_X,
    handler: "index.handler",
  },
  iotTopicRuleProps: {
    topicRulePayload: {
      ruleDisabled: false,
      description:
        "Processing of DTC messages from the AWS Connected Vehicle Solution.",
      sql: "SELECT * FROM 'connectedcar/dtc/#'",
      actions: [],
    },
  },
};

export class CdkStack extends Stack {
  constructor(scope: Construct, id: string, props?: StackProps) {
    super(scope, id, props);

    const queue = new sqs.Queue(this, "CdkQueue", {
      visibilityTimeout: Duration.seconds(300),
    });

    const topic = new sns.Topic(this, "CdkTopic");

    topic.addSubscription(new subs.SqsSubscription(queue));

    new IotToLambda(this, "test-iot-lambda-integration", constructProps);
  }
}
