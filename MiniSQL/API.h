#pragma once
#ifndef _API_H_
#define _API_H_

#include <string>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include "RecordManager.h"
#include "IndexManager.h"
#include "Interpreter.h"
#include "CatalogManager.h"
#include "SQL.h"
#include "Macro.h"

void Execute();
void ShowResult(Data data, Table tableinfor, vector<Attribute> column);

#endif