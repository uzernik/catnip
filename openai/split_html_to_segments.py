from bs4 import BeautifulSoup
import html
import re

def is_toc_item(content, sec, last_page_num):
    """
    Determine if a segment is a table-of-contents item based on the specified criteria.
    """
    words = content.split()
    if len(words) >= 1 and len(words) <= 10 and words[0] == sec:
        # Check if the last word is a valid page number
        try:
            page_num = int(words[-1])
            if page_num < 200 and page_num >= last_page_num:
                return True, page_num
        except ValueError:
            pass
    return False, last_page_num


class Document:
    def __init__(self, page_content: str, metadata: dict):
        self.page_content = page_content
        self.metadata = metadata

    def __repr__(self):
        return f"Document(page_content={self.page_content}, metadata={self.metadata})"


def split_html_on_div_attributes(file_name: str, max_level: int) -> list:
    """
    Reads an HTML file and splits it on <div> tags with lev, sec, and name attributes up to a specified max_level,
    excluding segments that are likely table-of-contents items.

    Args:
    file_name (str): The name of the HTML file.
    max_level (int): The maximum level of <div> tags to include.

    Returns:
    list: A list of Document objects, each representing a <div> segment.
    """
    # Read the HTML file
    with open(file_name, 'r', encoding='utf-8') as file:
        html_content = file.read()

    # Parse the HTML content
    soup = BeautifulSoup(html_content, 'html.parser')

    # Find all <div> elements with 'lev' attribute
    divs = [div for div in soup.find_all('div', attrs={'lev': True}) 
            if int(div.get('lev', 999)) <= max_level]

    # Creating list of Document objects for each match
    documents = []
    last_page_num = 0
    for sn, div in enumerate(divs, start=1):
        lev = div.get('lev', '')
        sec = div.get('sec', '')
        name = div.get('name', '')  # Extracting the 'name' attribute

        # Extracting text content, excluding nested <div> tags and HTML tags
        content = ''.join(child.get_text(separator=' ', strip=True) for child in div.children if child.name != 'div')

        is_toc, last_page_num = is_toc_item(content, sec, last_page_num)
        if not is_toc:
            # Creating a Document object if not a ToC item
            metadata = {"sn": sn, "lev": lev, "sec": sec, "name": name}
            document = Document(page_content=content, metadata=metadata)
            documents.append(document)
            if sn < 40:
                print(f'DDD {sn} {sec} NM={name} PC={content}')
            
        elif sn < 40:
            print(f'TOC {sn} {sec} NM={name} PC={content}')

    return documents
    


# Example usage
##max_level = 3
##file_name = '/media/assaf_demo/hsgatlin/starbucks/dsafile_00000427'  
##divs = split_html_on_div_attributes(file_name, max_level)
