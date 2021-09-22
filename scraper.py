from googleapiclient.discovery import build
from bs4 import BeautifulSoup

from urllib.request import Request, urlopen
from urllib.error import URLError
from http.client import InvalidURL
from urllib import robotparser
from collections import Counter, deque, defaultdict
from queue import Queue
from datetime import datetime
import threading
import sys

import credential


class Crawler:
    def __init__(self, page_num=200, max_thread=20, debug=False):
        self.page_required = page_num
        self.NOVELTY = page_num + 1
        self.IMPORTANCE = 0
        self.WORKER = True
        self.debug = debug

        # the novelty of a domain is decreased if a page is "crawled"
        self.novelty = defaultdict(lambda: self.NOVELTY + 1)
        self.parsed = {}                        # the parsed pages: url => [importance, timestamp]
        self.black_list = {}

        # the importance of a page is increased if it is "visited/encountered"
        self.visited = {}                       # the visited pages: url => imp_score
        self.importance = defaultdict(deque)    # the queues: imp_score => url
        self.max_importance = 0
        self.local_visited = threading.local()  # visiting times of a page

        self.thread_pool = Queue()
        self.thread_pool.queue = deque([self.WORKER for _ in range(max_thread)])
        self.thread_lock = threading.Lock()
        self.imp_lock = threading.Lock()

    @staticmethod
    def send_request(url: str) -> (BeautifulSoup, int):
        user_agent = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:91.0) Gecko/20100101 Firefox/91.0'
        try:
            http_request = Request(url, headers={'User-Agent': user_agent})
            http_response = urlopen(http_request)
            http_response = http_response.read()
        except URLError as e:
            # print(f'Error occurs when requesting {url}:')
            # print(f'\t{e}')
            return str(e)
        except InvalidURL:
            # print(f'Error occurs when requesting {url}')
            return None
        return BeautifulSoup(http_response, 'html.parser', from_encoding='utf-8'), len(http_response)

    def parse_seed(self, query: str) -> None:
        service = build('customsearch', 'v1', developerKey=credential.google_api_key)
        result = service.cse().list(q=query, cx=credential.google_cse_id, num='10').execute()['items']
        for res in result:
            self.spawn_parser(res['link'], 1)

        self.crawl()

        # seed_site = 'https://google.com/search?q='
        # if query.find('http') == -1:
        #     query = seed_site + '+'.join(query.split())
        # html, _ = send_request(query)
        # divs = html.find(id='search').find_all(attrs={'class': 'g'})
        # divs = html.select('#search div.g')
        #
        # for div in divs:
        #     url = div.a['href']
        #     if not url:
        #         continue

    def crawl(self) -> None:
        while len(self.parsed) < self.page_required and self.max_importance >= 0:
            if self.max_importance > 0:
                url = self.get_next_url()
                if self.debug:
                    print(f'{len(self.parsed)} parsed, {len(self.visited)} in queue,',
                          f'{threading.active_count()}/{len(self.thread_pool.queue)} in work',
                          f'next url: {url} from imp: {self.max_importance}/{len(self.importance[self.max_importance])}')
                if not url:
                    self.update_max_importance()  # locked
                    continue
                imp_score = self.visited.pop(url)
                self.importance[imp_score].remove(url)
                self.spawn_parser(url, imp_score)

    def spawn_parser(self, url, importance):
        self.thread_pool.get()
        t = threading.Thread(target=self.parse_page, args=(url, importance, ))
        t.daemon = True
        t.start()

    @staticmethod
    def get_domain(link: str) -> str:
        seg = link.split('://')
        return seg[0] + '://' + seg[1].split('/')[0] + '/'

    def get_next_url(self):
        url, novelty = '', float('-inf')
        for _url in self.importance[self.max_importance]:
            domain = Crawler.get_domain(_url)
            if self.novelty[domain] > novelty:
                url = _url
                novelty = self.novelty[domain]
        return url

    def parse_page(self, url: str, importance: int) -> None:
        """
        To be logged:
        * size (bytes)
        * depth (based on the ones queued)
        * priority score
            * importance
            * novelty
        * timestamp of being downloaded
        """
        if self.debug:
            print(f'parsing {url}')
        domain = Crawler.get_domain(url)
        self.novelty[domain] -= 1

        response = Crawler.send_request(url)
        if not response:
            self.thread_pool.put(self.WORKER)
            return
        elif type(response) == str:
            self.black_list[url] = response
            self.thread_pool.put(self.WORKER)
            return
        html, content_length = response
        self.parsed[url] = [importance,
                            self.novelty[domain] + 1,
                            datetime.now(),
                            content_length,
                            self.get_depth(url)]  # no lock required since GIL
        self.get_links(html, url)
        self.thread_pool.put(self.WORKER)

    def get_depth(self, url: str):
        depth = 0
        prefix = url.find('://')
        pos = url.find('/', prefix + 3)
        while pos != -1:
            parent = url[:pos + 1]
            if parent in self.visited or parent in self.parsed:
                depth += 1
            pos = url.find('/', pos + 1)
        return depth

    def get_links(self, html, curr_url: str) -> None:
        self.local_visited.counter = Counter()
        links = html.find_all('a')
        for anchor_tag in links:
            url = anchor_tag.get('href')
            if not Crawler.validate_url(url):
                continue
            url = self.complete_url(anchor_tag['href'], curr_url)
            if url in self.parsed:
                continue
            self.local_visited.counter[url] += 1

        # merge the local counter
        self.thread_lock.acquire()
        for url, count in self.local_visited.counter.items():
            self.integrate_page(url, count)
        self.thread_lock.release()

    @staticmethod
    def validate_url(url: str) -> bool:
        if not url or url == '/' or\
                url[0] == '?' or url[0] == '#' or\
                url.find('tel:') != -1 or\
                url.find('javascript:') != -1:
            return False
        return True

    def complete_url(self, link: str, parsing_url: str) -> str:
        link = link.split('#')[0]
        if link.find('://') != -1:
            return link
        if link[:1] == '/':
            return Crawler.get_domain(parsing_url) + link[1:]
        if link[:2] == './':
            return parsing_url + link[2:]
        i = link.rfind('../')
        if i != -1:
            # replace ../ with / from the last
            return self.complete_url('/' + link[:i] + link[i + 3:], parsing_url)
        return 'https://' + link

    def integrate_page(self, url: str, count: int) -> None:
        if url in self.visited:
            self.importance[self.visited[url]].remove(url)
            self.visited[url] += count
            self.importance[self.visited[url]].append(url)
            # self.update_max_importance(self.max_importance)
            self.max_importance = max(self.max_importance, self.visited[url])
        else:
            self.visited[url] = count
            self.importance[count].append(url)
            self.update_max_importance(count)
            # self.update_max_importance(self.max_importance)
            self.max_importance = max(self.max_importance, count)

    def update_max_importance(self, value=-1) -> None:
        if value > 0:
            self.max_importance = max(self.max_importance, value)
            return
        while self.max_importance >= 0 and not self.importance[self.max_importance]:
            self.max_importance -= 1


if __name__ == '__main__':
    debug_mode = False
    if len(sys.argv) == 2 and sys.argv[1] == 'debug':
        debug_mode = True

    keyword_url = input('Start with some keyword or url: ')
    crawler = Crawler(page_num=100, max_thread=50, debug=debug_mode)
    crawler.parse_seed(keyword_url)
    while 1:
        if len(crawler.parsed) >= 100:
            break

    if keyword_url.find('://') == -1:
        filename = '_'.join(keyword_url.split())
    else:
        filename = crawler.get_domain(keyword_url).split('://')[1].split('/')[0]

    filename = 'log_' + filename
    with open(filename, 'w') as file:
        result = crawler.parsed.copy()
        file.write('link, (importance, novelty), downloading time, size, depth\n')
        for link in result:
            input_str = ', '.join(map(str, [link, *result[link]]))
            if debug_mode:
                print(input_str)
            file.write(input_str + '\n')

    with open(filename + '_blacklist', 'w') as file:
        blacklist = crawler.black_list.copy()
        for link in blacklist:
            input_str = f'{link}: {blacklist[link]}'
            if debug_mode:
                print(input_str)
            file.write(input_str + '\n')