#include "statistics.h"

#include <cstdio>
#include <map>
#include <string>

std::map<std::string, StatisticHolder*>* statistics;

void DumpStatisticsToStderr() {
  EnsureStatistics();

  fprintf(stderr, "{");
  bool first = true;
  for (auto statistic : *statistics) {
    if (!statistic.second->ShouldDump()) {
      continue;
    }

    if (first) {
      first = false;
    } else {
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "'%s': %s", 
        statistic.first.c_str(),
        statistic.second->Dump().c_str());
  }
  fprintf(stderr, "}\n");
}

