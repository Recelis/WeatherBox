#!/usr/bin/env node
import * as cdk from "aws-cdk-lib";
import { CdkStack, stackName } from "../lib/cdk-stack";

const app = new cdk.App();
new CdkStack(app, stackName);
