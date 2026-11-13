#!/usr/bin/env python3
"""Publish a small CI receipt through Git; no tokens or raw build logs are stored."""
import json
import os
import re
import subprocess
from pathlib import Path


def git(*args, data=None):
    return subprocess.check_output(['git', *args], input=data, text=True).strip()


def main():
    tag = os.environ['RELEASE_TAG']
    if not re.fullmatch(r'v\d+\.\d+\.\d+', tag):
        raise ValueError('Invalid tag')
    platform = os.environ.get('REPORT_PLATFORM', '')
    if platform and platform not in ('linux-x86_64', 'windows-x86_64', 'macos-universal'):
        raise ValueError('Invalid platform')
    branch = ('ci-build-' + tag + '-' + platform) if platform else ('ci-release-' + tag)
    report = {'tag': tag, 'automation_commit': os.environ['GITHUB_SHA'],
              'run_id': os.environ['GITHUB_RUN_ID'],
              'run_url': os.environ['LOGISTH_RUN_URL'],
              'phase': os.environ.get('REPORT_PHASE', 'building')}
    if os.environ.get('JOB_RESULTS'):
        report['jobs'] = json.loads(os.environ['JOB_RESULTS'])
    if platform:
        report['platform'] = platform
        info = Path('native-out') / f'build-info-{platform}.json'
        if info.is_file():
            report['build'] = json.loads(info.read_text())
    failures=[]
    for log in sorted(Path('native-out' if platform else 'ci-artifacts').rglob('*.log')):
        messages=[]
        for line in log.read_text(errors='replace').splitlines():
            if re.search(r'error[: ]|fatal error|CMake Error|FAILED:',line,re.I):
                line=re.sub(r'https?://[^/@ ]+@','https://[redacted]@',line)
                line=re.sub(r'(gh[pousr]_[A-Za-z0-9_]+|github_pat_[A-Za-z0-9_]+)','[redacted]',line)
                messages.append(line[:400])
        if messages:failures.append({'log':str(log),'errors':messages[-12:]})
    if failures:report['failures']=failures
    result = Path('native-release-result.json')
    if result.is_file():
        report.update(json.loads(result.read_text()))
    blob = git('hash-object', '-w', '--stdin', data=json.dumps(report, indent=2) + '\n')
    tree = git('mktree', '-z', data=f'100644 blob {blob}\tstatus.json\0')
    remote = git('ls-remote', '--heads', 'origin', f'refs/heads/{branch}')
    parents = []
    if remote:
        parent = remote.split()[0]
        git('fetch', '--depth', '1', 'origin', f'refs/heads/{branch}')
        parents = ['-p', parent]
    git('config', 'user.name', 'github-actions[bot]')
    git('config', 'user.email', '41898282+github-actions[bot]@users.noreply.github.com')
    commit = git('commit-tree', tree, *parents,
                 data=f'CI status for {tag}: {report["phase"]}\n')
    git('push', 'origin', f'{commit}:refs/heads/{branch}')
    print('CI receipt:', branch, report['phase'])


if __name__ == '__main__':
    main()
