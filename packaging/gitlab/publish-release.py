#!/usr/bin/env python3
"""Upload durable release assets and publish a prepared GitLab release from CI."""
import json
import os
import sys
from pathlib import Path
from urllib.error import HTTPError
from urllib.parse import quote
from urllib.request import Request, urlopen

api = f"{os.environ['CI_API_V4_URL']}/projects/{os.environ['CI_PROJECT_ID']}"
tag = os.environ['CI_COMMIT_TAG']
headers = {'JOB-TOKEN': os.environ['CI_JOB_TOKEN']}


def request(path, method='GET', data=None, content_type='application/json'):
    req = Request(api + path, data=data, method=method,
                  headers={**headers, 'Content-Type': content_type})
    with urlopen(req, timeout=300) as response:
        return json.load(response)


publish = "--publish" in sys.argv
release_path = '/releases/' + quote(tag, safe='')
try:
    existing = request(release_path)
except HTTPError as error:
    if error.code != 404:
        raise
    existing = None

if existing and not existing.get('upcoming_release', False) and not publish:
    raise SystemExit('Refusing to replace an already published release during preparation')
if publish:
    # A delayed manual job must never replace Pages with an older app version.
    def version(value):
        parts = value.removeprefix('v').split('.')
        return tuple(map(int, parts)) if len(parts) == 3 and all(p.isdigit() for p in parts) else ()
    for release in request('/releases?per_page=100'):
        if not release.get('upcoming_release', False) and version(release['tag_name']) > version(tag):
            raise SystemExit('A newer release is already published; refusing to downgrade the repository')

links = []
for name in ['ProjectNotes.flatpak', 'projectnotes.flatpakrepo']:
    path = Path('packaging/linux') / name
    endpoint = f'/packages/generic/projectnotes/{quote(tag, safe="")}/{name}'
    if not publish:
        request(endpoint, 'PUT', path.read_bytes(), 'application/octet-stream')
    links.append({'name': name, 'url': api + endpoint,
                  'direct_asset_path': '/' + name, 'link_type': 'package'})

# Preserve any release notes already reviewed on GitLab.
notes = Path('packaging/gitlab/RELEASE_NOTES.md').read_text()
from datetime import datetime, timezone
payload = {'name': f'Project Notes {tag.removeprefix("v")}',
           'description': existing['description'] if existing else notes,
           'released_at': datetime.now(timezone.utc).isoformat() if publish else '2099-01-01T00:00:00Z'}
if existing:
    request(release_path, 'PUT', json.dumps(payload).encode())
else:
    payload.update(tag_name=tag)
    request('/releases', 'POST', json.dumps(payload).encode())

current_links = request(release_path)['assets']['links']
for link in links:
    match = next((item for item in current_links if item['name'] == link['name']), None)
    endpoint = release_path + '/assets/links'
    if match:
        endpoint += '/' + str(match['id'])
    request(endpoint, 'PUT' if match else 'POST', json.dumps(link).encode())
print(f'{"Published" if publish else "Prepared upcoming release"} {tag} with Flatpak assets')
