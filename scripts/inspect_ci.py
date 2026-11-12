#!/usr/bin/env python3
"""Read infrastructure and job conclusions without copying credentials or raw logs."""
import json
import os
import re
import subprocess
import sys
from pathlib import Path


def api(path):
    result = subprocess.run(['gh', 'api', path], capture_output=True, text=True)
    if result.returncode:
        return {'error': result.stderr[:600]}
    return json.loads(result.stdout)


def safe(value):
    text = str(value or '')
    text = re.sub(r'(gh[pousr]_[A-Za-z0-9_]+|github_pat_[A-Za-z0-9_]+)', '[redacted]', text)
    text = re.sub(r'https?://[^/@ ]+@', 'https://[redacted]@', text)
    return text[:2500]


repository = os.environ['GITHUB_REPOSITORY']
run_id = sys.argv[1]
if not run_id.isdigit():
    raise SystemExit('Invalid run id')
run = api(f'repos/{repository}/actions/runs/{run_id}')
jobs = api(f'repos/{repository}/actions/runs/{run_id}/jobs?per_page=100')
report = {key: run.get(key) for key in ['id', 'status', 'conclusion', 'head_sha', 'event',
                                       'run_started_at', 'updated_at', 'html_url', 'run_attempt']}
report['jobs'] = []
for job in jobs.get('jobs', []):
    item = {key: job.get(key) for key in ['id', 'name', 'status', 'conclusion', 'started_at',
                                       'completed_at', 'runner_name', 'runner_group_name', 'labels']}
    item['steps'] = [{key: step.get(key) for key in ['name', 'status', 'conclusion', 'number']}
                     for step in job.get('steps', [])]
    if job.get('check_run_url'):
        check = api(job['check_run_url'])
        output = check.get('output', {})
        item['check_output'] = {key: safe(output.get(key)) for key in ['title', 'summary', 'text']}
        if check.get('id'):
            annotations = api(f'repos/{repository}/check-runs/{check["id"]}/annotations?per_page=100')
            if isinstance(annotations, list):
                item['annotations'] = [{key: safe(a.get(key)) for key in ['title', 'message', 'path']}
                                       for a in annotations[:20]]
    report['jobs'].append(item)
if 'error' in run:
    report['error'] = safe(run['error'])
Path('ci-diagnostics.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
